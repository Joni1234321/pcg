module;

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
export module pcg.g_meshdemo;

import std;
// ============================================================================
//  g_meshdemo.cpp  --  Mesh batching demo with mixed quads + hex + text.
//
//  Self-contained SDL3 sample showing how to:
//    * build a vertex/index buffer with MakeQuad / MakeHex / MakeText helpers
//    * pack glyphs into a single texture atlas (with a white pixel for
//      untextured geometry) so that everything renders in ONE draw call
//    * track batch boundaries on texture changes and flush with
//      SDL_RenderGeometry
//
//  Build it standalone (it does not depend on the pcg engine). It expects
//  SDL3 + SDL3_ttf to be linkable. To plug into the existing project, add a
//  forward decl `void RunMeshDemo();` and call it from main, OR compile this
//  file on its own with:
//      g++ g_meshdemo.cpp -lSDL3 -lSDL3_ttf -o meshdemo
// ============================================================================

namespace meshdemo {

// ---------------------------------------------------------------------------
// Glyph atlas: one SDL_Texture holding every printable ASCII glyph in a row,
// plus a 2x2 white pixel block at UV origin so untextured geometry can share
// the same texture (and therefore the same batch).
// ---------------------------------------------------------------------------
struct Glyph {
    SDL_FRect uv;    // 0..1 atlas coords
    float w, h;      // pixel size on screen
    float bearing_x; // offset from pen X to glyph left
    float bearing_y; // offset from pen Y (baseline) to glyph top
    float advance;   // pen advance after drawing
};

struct GlyphAtlas {
    SDL_Texture* texture = nullptr;
    int atlas_w = 0;
    int atlas_h = 0;
    float line_height = 0.f;
    float ascent = 0.f;     // baseline offset from top of surface
    SDL_FRect white_uv { }; // tiny opaque region for solid fills
    std::array<Glyph, 128> glyphs { };

    static constexpr int FIRST_GLYPH = 32; // space
    static constexpr int LAST_GLYPH = 126; // ~
    static constexpr int WHITE_PAD_PX = 4; // size of solid-white block

    bool Build(SDL_Renderer* r, const char* font_path, float pt) {
        TTF_Font* font = TTF_OpenFont(font_path, pt);
        if (!font) {
            SDL_Log("TTF_OpenFont failed: %s", SDL_GetError());
            return false;
        }
        line_height = (float)TTF_GetFontHeight(font);
        ascent = (float)TTF_GetFontAscent(font);

        // --- Pass 1: render every glyph to its own surface, measure total width
        std::vector<SDL_Surface*> surfaces(128, nullptr);
        std::vector<int> minx(128, 0), miny(128, 0);
        std::vector<int> advance(128, 0);

        int total_w = WHITE_PAD_PX; // leave room for white block
        int max_h = WHITE_PAD_PX;
        SDL_Color white { 255, 255, 255, 255 };

        for (int c = FIRST_GLYPH; c <= LAST_GLYPH; ++c) {
            char buf[2] = { (char)c, 0 };
            SDL_Surface* s = TTF_RenderText_Blended(font, buf, 1, white);
            if (!s) { continue; }
            surfaces[c] = s;

            int mnx = 0, mxx = 0, mny = 0, mxy = 0, adv = 0;
            TTF_GetGlyphMetrics(font, (Uint32)c, &mnx, &mxx, &mny, &mxy, &adv);
            minx[c] = mnx;
            miny[c] = mxy; // top bearing from baseline
            advance[c] = adv;

            total_w += s->w + 1;
            if (s->h > max_h) { max_h = s->h; }
        }

        // --- Allocate atlas surface and blit
        SDL_Surface* atlas = SDL_CreateSurface(total_w, max_h, SDL_PIXELFORMAT_RGBA32);
        if (!atlas) {
            SDL_Log("CreateSurface failed: %s", SDL_GetError());
            TTF_CloseFont(font);
            return false;
        }
        // White block at (0,0)
        SDL_Rect wr { 0, 0, WHITE_PAD_PX, WHITE_PAD_PX };
        SDL_FillSurfaceRect(atlas, &wr, SDL_MapSurfaceRGBA(atlas, 255, 255, 255, 255));
        white_uv = { 1.0f / total_w, 1.0f / max_h, 1.0f / total_w, 1.0f / max_h }; // 1px inside the block

        int pen_x = WHITE_PAD_PX;
        for (int c = FIRST_GLYPH; c <= LAST_GLYPH; ++c) {
            SDL_Surface* s = surfaces[c];
            if (!s) { continue; }
            SDL_Rect dst { pen_x, 0, s->w, s->h };
            SDL_BlitSurface(s, nullptr, atlas, &dst);

            Glyph& g = glyphs[c];
            g.uv.x = (float)pen_x / total_w;
            g.uv.y = 0.f;
            g.uv.w = (float)s->w / total_w;
            g.uv.h = (float)s->h / max_h;
            g.w = (float)s->w;
            g.h = (float)s->h;
            g.bearing_x = (float)minx[c];
            g.bearing_y = (float)miny[c];
            g.advance = (float)advance[c];

            pen_x += s->w + 1;
            SDL_DestroySurface(s);
        }

        texture = SDL_CreateTextureFromSurface(r, atlas);
        // LINEAR filtering lets us downscale glyphs to any size without
        // aliasing. NEAREST would be sharper for pixel-art UI but ugly when
        // shrunk. Bake the atlas LARGER than the largest size you'll draw at.
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
        atlas_w = total_w;
        atlas_h = max_h;
        SDL_DestroySurface(atlas);
        TTF_CloseFont(font);
        return texture != nullptr;
    }

    const Glyph& Get(char c) const {
        unsigned u = (unsigned)c;
        if (u < FIRST_GLYPH || u > LAST_GLYPH) { u = '?'; }
        return glyphs[u];
    }

    float MeasureWidth(std::string_view s) const {
        float x = 0.f;
        for (char c : s) { x += Get(c).advance; }
        return x;
    }
};

// ---------------------------------------------------------------------------
// MeshBuilder: the heart of the demo.
// Records vertices, indices, and batch boundaries. Flush() issues one
// SDL_RenderGeometry call per batch.
// ---------------------------------------------------------------------------
class MeshBuilder {
public:
    void MakeQuad(SDL_FRect dst, SDL_FRect uv, SDL_FColor col, SDL_Texture* tex) {
        SetTexture(tex);
        int base = (int)verts_.size();
        verts_.push_back({ { dst.x, dst.y }, col, { uv.x, uv.y } });
        verts_.push_back({ { dst.x + dst.w, dst.y }, col, { uv.x + uv.w, uv.y } });
        verts_.push_back({ { dst.x + dst.w, dst.y + dst.h }, col, { uv.x + uv.w, uv.y + uv.h } });
        verts_.push_back({ { dst.x, dst.y + dst.h }, col, { uv.x, uv.y + uv.h } });
        PushTri(base + 0, base + 1, base + 2);
        PushTri(base + 0, base + 2, base + 3);
    }

    // Solid quad using the white pixel from the atlas (still same texture -> same batch).
    void MakeSolidQuad(SDL_FRect dst, SDL_FColor col, const GlyphAtlas& atlas) { MakeQuad(dst, atlas.white_uv, col, atlas.texture); }

    // Filled hexagon (pointy top) using the white pixel.
    void MakeHex(SDL_FPoint center, float radius, SDL_FColor col, const GlyphAtlas& atlas) {
        SetTexture(atlas.texture);
        int base = (int)verts_.size();
        SDL_FPoint uv { atlas.white_uv.x, atlas.white_uv.y };
        verts_.push_back({ center, col, uv });
        for (int i = 0; i < 6; ++i) {
            float a = (float)i * (SDL_PI_F / 3.f) - SDL_PI_F / 2.f; // pointy top
            verts_.push_back({ { center.x + SDL_cosf(a) * radius, center.y + SDL_sinf(a) * radius }, col, uv });
        }
        for (int i = 0; i < 6; ++i) { PushTri(base, base + 1 + i, base + 1 + ((i + 1) % 6)); }
    }

    void MakeText(std::string_view s, SDL_FPoint pos, const GlyphAtlas& atlas, SDL_FColor col) { MakeTextScaled(s, pos, atlas, col, 1.f); }

    // Render text at an arbitrary on-screen scale. `scale` is a multiplier on
    // the baked atlas size -- e.g. atlas baked at 48pt, scale=0.5 gives 24px
    // text. The atlas is sampled with LINEAR so any scale looks smooth.
    //
    // IMPORTANT: TTF_RenderText_Blended produces a surface sized to the full
    // line height with the glyph already positioned at the font's baseline
    // inside it. So we offset every glyph by the same ascent value -- using
    // per-glyph bearing_y here would make text wavy (different y per letter).
    void MakeTextScaled(std::string_view s, SDL_FPoint pos, const GlyphAtlas& atlas, SDL_FColor col, float scale) {
        SetTexture(atlas.texture);
        float pen_x = pos.x;
        float top_y = pos.y; // top of the text line
        for (char c : s) {
            const Glyph& g = atlas.Get(c);
            SDL_FRect dst { pen_x, top_y, g.w * scale, g.h * scale };
            MakeQuad(dst, g.uv, col, atlas.texture);
            pen_x += g.advance * scale;
        }
    }

    // Convenience: pick scale so a line of text ends up `target_px` tall.
    void MakeTextAtPixelHeight(std::string_view s, SDL_FPoint pos, const GlyphAtlas& atlas, SDL_FColor col, float target_px) {
        float scale = target_px / atlas.line_height;
        MakeTextScaled(s, pos, atlas, col, scale);
    }

    // A boxed label: filled background quad + centered text. Pass tex_px=0
    // for a transparent (no background) label.
    void MakeLabel(std::string_view text, SDL_FPoint pos, const GlyphAtlas& atlas, SDL_FColor text_col, SDL_FColor bg_col, float text_px, float pad_px = 6.f) {
        float scale = text_px / atlas.line_height;
        float text_w = atlas.MeasureWidth(text) * scale;
        float text_h = atlas.line_height * scale;
        SDL_FRect box { pos.x, pos.y, text_w + pad_px * 2.f, text_h + pad_px * 2.f };
        if (bg_col.a > 0.f) { MakeSolidQuad(box, bg_col, atlas); }
        MakeTextScaled(text, { pos.x + pad_px, pos.y + pad_px }, atlas, text_col, scale);
    }

    void Flush(SDL_Renderer* r) {
        for (auto& b : batches_) {
            if (b.idx_count == 0) { continue; }
            SDL_RenderGeometry(r, b.tex, verts_.data(), (int)verts_.size(), indices_.data() + b.idx_offset, b.idx_count);
        }
        ++flush_count_;
    }

    void Clear() {
        verts_.clear();
        indices_.clear();
        batches_.clear();
        current_tex_ = nullptr;
    }

    int BatchCount() const { return (int)batches_.size(); }
    int VertexCount() const { return (int)verts_.size(); }
    int IndexCount() const { return (int)indices_.size(); }

private:
    struct Batch {
        SDL_Texture* tex;
        int idx_offset;
        int idx_count;
    };

    void SetTexture(SDL_Texture* t) {
        if (!batches_.empty() && t == current_tex_) { return; }
        batches_.push_back({ t, (int)indices_.size(), 0 });
        current_tex_ = t;
    }

    void PushTri(int a, int b, int c) {
        indices_.push_back(a);
        indices_.push_back(b);
        indices_.push_back(c);
        batches_.back().idx_count += 3;
    }

    std::vector<SDL_Vertex> verts_;
    std::vector<int> indices_;
    std::vector<Batch> batches_;
    SDL_Texture* current_tex_ = nullptr;
    int flush_count_ = 0;
};

// ---------------------------------------------------------------------------
// A tiny "counter widget" -- mixes a background quad + border hexes + text.
// All of it lands in ONE batch because everything uses the glyph atlas.
// ---------------------------------------------------------------------------
static void DrawCounter(MeshBuilder& mb, const GlyphAtlas& atlas, SDL_FPoint pos, int value, std::string_view label) {
    constexpr SDL_FColor BG { 0.10f, 0.12f, 0.18f, 0.95f };
    constexpr SDL_FColor BORDER { 0.35f, 0.55f, 0.85f, 1.00f };
    constexpr SDL_FColor TEXT { 0.95f, 0.95f, 0.95f, 1.00f };
    constexpr SDL_FColor ACCENT { 1.00f, 0.78f, 0.20f, 1.00f };

    const float W = 220.f, H = 90.f;
    SDL_FRect rect { pos.x, pos.y, W, H };

    // 1. background panel (solid quad sampling the white pixel)
    mb.MakeSolidQuad(rect, BG, atlas);

    // 2. decorative hex "gems" along the top
    for (int i = 0; i < 5; ++i) {
        SDL_FPoint c { pos.x + 18.f + i * 22.f, pos.y + 14.f };
        mb.MakeHex(c, 7.f, (i < value % 6) ? ACCENT : BORDER, atlas);
    }

    // 3. label and value text (same atlas -> still no batch break)
    char num[32];
    std::snprintf(num, sizeof(num), "%d", value);

    // Counter text is rendered at FIXED pixel heights -- the whole counter
    // belongs to the HUD layer and never scales with world zoom.
    mb.MakeTextAtPixelHeight(label, { pos.x + 12.f, pos.y + 22.f }, atlas, TEXT, 16.f);
    mb.MakeTextAtPixelHeight(num, { pos.x + 12.f, pos.y + 44.f }, atlas, ACCENT, 32.f);
}

// ---------------------------------------------------------------------------
// Demonstrates an INTENTIONAL batch break: drawing a quad from a second
// texture forces a new SDL_RenderGeometry call. We synthesize a checkerboard
// texture so the demo stays self-contained.
// ---------------------------------------------------------------------------
static SDL_Texture* MakeCheckerTexture(SDL_Renderer* r) {
    constexpr int N = 64;
    SDL_Surface* s = SDL_CreateSurface(N, N, SDL_PIXELFORMAT_RGBA32);
    Uint32* px = (Uint32*)s->pixels;
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            bool dark = ((x / 8) + (y / 8)) & 1;
            Uint8 v = dark ? 60 : 200;
            px[y * (s->pitch / 4) + x] = SDL_MapSurfaceRGBA(s, v, v, (Uint8)(v / 2 + 60), 255);
        }
    }
    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
    SDL_SetTextureScaleMode(t, SDL_SCALEMODE_NEAREST);
    SDL_DestroySurface(s);
    return t;
}

} // namespace meshdemo

// ---------------------------------------------------------------------------
// Entry point. Rename to `RunMeshDemo` and remove `main` if you wire this
// into the pcg engine.
// ---------------------------------------------------------------------------
int main(int /*argc*/, char* /*argv*/[]) {
    using namespace meshdemo;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }
    if (!TTF_Init()) {
        SDL_Log("TTF_Init: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* win = nullptr;
    SDL_Renderer* rdr = nullptr;
    if (!SDL_CreateWindowAndRenderer("SDL3 Mesh Batching Demo", 900, 600, 0, &win, &rdr)) {
        SDL_Log("CreateWindowAndRenderer: %s", SDL_GetError());
        return 1;
    }

    GlyphAtlas atlas;
    // Bake at LARGE size (48pt). We will draw text at a variety of pixel
    // heights by scaling quads -- LINEAR filtering keeps it smooth.
    if (!atlas.Build(rdr, "assets/Courier_Prime/CourierPrime-Bold.ttf", 48.f)) {
        SDL_Log("Atlas build failed -- run from the project root so the "
                "relative font path resolves.");
        return 1;
    }
    SDL_Texture* checker = MakeCheckerTexture(rdr);

    MeshBuilder mb;
    bool running = true;
    Uint64 start_ns = SDL_GetTicksNS();
    int frame = 0;
    float frame_avg = 0.f;

    // World-space camera zoom. Affects hexes ("the game world") but NOT
    // counters/HUD, which always render at fixed pixel sizes.
    float zoom = 1.f;
    SDL_FPoint pan { 0.f, 0.f }; // world-space pan offset
    bool dragging = false;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) { running = false; }
            if (ev.type == SDL_EVENT_KEY_DOWN) {
                if (ev.key.key == SDLK_ESCAPE) { running = false; }
                if (ev.key.key == SDLK_EQUALS || ev.key.key == SDLK_PLUS) { zoom *= 1.1f; }
                if (ev.key.key == SDLK_MINUS) { zoom /= 1.1f; }
                if (ev.key.key == SDLK_0) {
                    zoom = 1.f;
                    pan = { 0, 0 };
                }
                const float STEP = 30.f;
                if (ev.key.key == SDLK_LEFT || ev.key.key == SDLK_A) { pan.x += STEP; }
                if (ev.key.key == SDLK_RIGHT || ev.key.key == SDLK_D) { pan.x -= STEP; }
                if (ev.key.key == SDLK_UP || ev.key.key == SDLK_W) { pan.y += STEP; }
                if (ev.key.key == SDLK_DOWN || ev.key.key == SDLK_S) { pan.y -= STEP; }
            }
            if (ev.type == SDL_EVENT_MOUSE_WHEEL) { zoom *= (ev.wheel.y > 0 ? 1.1f : 1.f / 1.1f); }
            if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN && (ev.button.button == SDL_BUTTON_MIDDLE || ev.button.button == SDL_BUTTON_RIGHT)) { dragging = true; }
            if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP && (ev.button.button == SDL_BUTTON_MIDDLE || ev.button.button == SDL_BUTTON_RIGHT)) { dragging = false; }
            if (ev.type == SDL_EVENT_MOUSE_MOTION && dragging) {
                pan.x += ev.motion.xrel;
                pan.y += ev.motion.yrel;
            }
        }
        if (zoom < 0.2f) { zoom = 0.2f; }
        if (zoom > 5.0f) { zoom = 5.0f; }

        Uint64 now_ns = SDL_GetTicksNS();
        float t_sec = (now_ns - start_ns) / 1e9f;

        SDL_SetRenderDrawColor(rdr, 24, 26, 32, 255);
        SDL_RenderClear(rdr);

        mb.Clear();

        // ---- BATCH 1: everything that uses the glyph atlas -----------------
        // header bar -- screen-space, fixed size, immune to zoom
        mb.MakeSolidQuad({ 20, 20, 860, 60 }, { 0.15f, 0.18f, 0.24f, 1.f }, atlas);
        mb.MakeTextAtPixelHeight("SDL3 mesh batching demo  --  wheel/+- zoom, WASD or drag to pan", { 36, 30 }, atlas, { 0.95f, 0.95f, 0.95f, 1.f }, 18.f);

        // --- WORLD-SPACE LAYER -------------------------------------------
        // A row of animated hexes anchored at world origin (450,140). Their
        // size and offsets are multiplied by `zoom` and shifted by `pan`,
        // so zooming and panning move the world.
        SDL_FPoint world_origin { 450.f + pan.x, 140.f + pan.y };
        for (int i = 0; i < 24; ++i) {
            float local_x = (i - 11.5f) * 34.f;
            float local_y = SDL_sinf(t_sec * 2.f + i * 0.4f) * 14.f;
            SDL_FColor c { 0.5f + 0.5f * SDL_sinf(t_sec + i * 0.3f), 0.5f + 0.5f * SDL_sinf(t_sec + i * 0.3f + 2.f), 0.5f + 0.5f * SDL_sinf(t_sec + i * 0.3f + 4.f), 1.f };
            SDL_FPoint world_pos { world_origin.x + local_x * zoom, world_origin.y + local_y * zoom };
            mb.MakeHex(world_pos, 14.f * zoom, c, atlas);
        }

        // Floating world-space label: scales with zoom along with the hexes.
        // MakeLabel = background box + text in one helper.
        mb.MakeLabel("world-space (scales with zoom)", { world_origin.x - 140.f * zoom, world_origin.y + 28.f * zoom }, atlas, { 0.95f, 0.95f, 1.0f, 1.f }, { 0.10f, 0.14f, 0.22f, 0.85f }, 14.f * zoom, 6.f * zoom);

        // --- SCREEN-SPACE HUD LAYER --------------------------------------
        // The counters stay the same on-screen size regardless of zoom.
        DrawCounter(mb, atlas, { 40, 200 }, (int)(t_sec * 5) % 1000, "SCORE");
        DrawCounter(mb, atlas, { 280, 200 }, (int)(t_sec * 11) % 1000, "COMBO");
        DrawCounter(mb, atlas, { 520, 200 }, (int)(t_sec * 2) % 1000, "LIVES");

        // A small panel demonstrating multiple text sizes from ONE atlas.
        mb.MakeSolidQuad({ 760, 200, 120, 90 }, { 0.15f, 0.18f, 0.24f, 1.f }, atlas);
        mb.MakeTextAtPixelHeight("TINY", { 770, 206 }, atlas, { 0.95f, 0.95f, 0.95f, 1.f }, 10.f);
        mb.MakeTextAtPixelHeight("SMALL", { 770, 222 }, atlas, { 0.95f, 0.95f, 0.95f, 1.f }, 14.f);
        mb.MakeTextAtPixelHeight("BIG", { 770, 244 }, atlas, { 1.0f, 0.78f, 0.20f, 1.f }, 32.f);

        // ---- BATCH 2: a second texture forces a new SDL_RenderGeometry -----
        mb.MakeQuad({ 40, 320, 200, 200 }, { 0.f, 0.f, 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f }, checker);

        // ---- BATCH 3: back to the atlas (text overlay on the checker) ------
        mb.MakeText("texture swap = new batch", { 50, 330 }, atlas, { 1.f, 1.f, 0.2f, 1.f });

        // ---- BATCH 1 continued logically, but split because of batch 2 ----
        // status text (screen-space, fixed size)
        char stats[200];
        std::snprintf(stats, sizeof(stats), "zoom:%.2fx  pan:(%.0f,%.0f)  batches:%d  verts:%d  frame:%.2f ms", zoom, pan.x, pan.y, mb.BatchCount(), mb.VertexCount(), frame_avg);
        mb.MakeTextAtPixelHeight(stats, { 40, 560 }, atlas, { 0.7f, 0.85f, 1.f, 1.f }, 14.f);

        mb.Flush(rdr);
        SDL_RenderPresent(rdr);

        // simple frame-time average
        ++frame;
        if (frame % 30 == 0) {
            Uint64 elapsed_ns = SDL_GetTicksNS() - start_ns;
            frame_avg = (float)elapsed_ns / 1e6f / frame;
        }
    }

    SDL_DestroyTexture(checker);
    SDL_DestroyTexture(atlas.texture);
    SDL_DestroyRenderer(rdr);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
