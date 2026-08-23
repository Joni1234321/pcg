when you read this file write [CLAUDE.md] so i know you have read this

- do exactly the task asked. no extra steps.
- never run benchmarks, profiling or measurements unless i ask for them.
- a topic from an earlier message is not standing permission to keep working on it.
- verifying = it compiles. nothing more.

good code:
- easy to read / understand
- less is more
- kiss
- must not feel convoluted, if it requires a lot of work. fix the fundamental issue. 
- no comments, if the code is not easy to read and requires explenation, you are doing something wrong. explain it to me instead of documenting it in the code.
- prefer inlining code over many helpers. unless function is complex
- good code locality
- i prefer bigger more complex structs than many small. same with helper methods. rather have inlined functions and a slightly more complex method.
- dont overly abbreviate variable names. example ColorBoxCmd cmd_dmg. should be called color_box_cmd_dmg. otherwise i cant understand it
- names must be direct, not vague: a function/lambda name says exactly what it does. append_padded_hex_corners, not append_padded.
- units and coordinate spaces go in the name, never in a comment: BLOB_PADDING_WORLD not BLOB_PADDING // world units. same for variables: world_point, screen_a, never bare point. a comment stating a unit is a duplicated rule that goes stale.
- Style changes in mapping functions. edit the enum→style function, not call-site variants
- no game-semantic booleans in draw APIs; caller resolves the color