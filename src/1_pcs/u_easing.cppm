module;

export module pcs.easing;

import pce.std;
import pce.math;

export namespace hex {
inline f32 EaseInSine(const f32 t) { return 1.0F - math::Cos(t * math::PI * 0.5F); }
inline f32 EaseOutSine(const f32 t) { return math::Sin(t * math::PI * 0.5F); }
inline f32 EaseInOutSine(const f32 t) { return -(math::Cos(math::PI * t) - 1.0F) * 0.5F; }
inline f32 EaseInCubic(const f32 t) { return t * t * t; }
inline f32 EaseOutCubic(const f32 t) { return 1.0F - math::Pow(1.0F - t, 3.0F); }
inline f32 EaseInQuart(const f32 t) { return t * t * t * t; }
inline f32 EaseOutQuart(const f32 t) { return 1.0F - math::Pow(1.0F - t, 4.0F); }
} // namespace hex
