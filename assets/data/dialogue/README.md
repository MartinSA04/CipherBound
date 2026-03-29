# NPC Dialogue Files

NPC dialogue can now live in separate `.npcdialogue` files instead of long inline map rows.

In a `.map` file, point an NPC at a dialogue file like this:

```text
npc_id,sprite,name,normal,false,4,6,dialogue:assets/data/dialogue/pallet_town/example.npcdialogue,down,0
```

Format rules for `.npcdialogue` files:

- One dialogue line per physical line.
- Blank lines are ignored.
- Lines starting with `#` are comments.
- Use `[default]` for the fallback stage.
- Use `[flag some_flag]` for dialogue that should appear when a flag is set.
- Stage order matters. The first matching stage is used.
- Lines before the first header are treated as the default stage.

Example:

```text
# Default dialogue
Welcome to Viridian.
The faculty is north of here.

[flag beat_faculty_1]
So you really did it.
Marius is not easy to beat.

[default]
Come back when you are ready.
```

The old inline map dialogue format still works, but separate files are preferred for maintainability.
