# Claude View (nice!view) animations

This repo drives the **mascot / status** bitmaps on the `claude_view` shield from **frame directories** plus a **TOML manifest**. The generated C file is large; changing the manifest and regenerating is the normal way to add, reorder, or temporarily drop animations.

Use this doc when you need to **change which animations ship in firmware**, **fix wrong animation after a key press**, or **understand how indices line up** across manifest, generated code, and keymap.

---

## Mental model

1. **`tools/animations.toml`** — Ordered list of `[[animations]]` tables. **Top-to-bottom order is the runtime index:** first entry is `0`, second is `1`, etc.
2. **`python3 tools/nicview_frames.py --manifest … --output …`** — Reads the manifest, processes images, writes **`boards/shields/claude_view/assets/claude_art.c`**. That file defines `animations[]` and **`animation_count`** (must match the number of active `[[animations]]` blocks).
3. **`config/eyelash_corne.keymap`** — `zmk,behavior-anim-set` nodes use **`animation-index = <N>`** where **N must match the manifest order** for that animation at build time.
4. **`boards/shields/claude_view/behaviors/anim_select.c`** — **`INITIAL_ANIM`** is the 0-based index shown at boot (usually `0` = first manifest entry).
5. **`behavior_anim_cycle.c`** — `&anim_cyc` advances `(current + 1) % animation_count`; no manual index list to edit.

If you remove or comment out a middle entry in the manifest, **every later animation shifts down one index**. You must update **all** affected `animation-index` properties and comments, then regenerate `claude_art.c`.

---

## Prerequisites

- **Python 3.11+** (or older Python with **`tomli`** installed for TOML parsing).
- **ImageMagick** (`convert` / `magick` on `PATH`) — used for crop, resize, dither, rotate.
- Run the generator from the **repository root** so relative `dir` paths in the manifest resolve correctly.

```bash
cd /path/to/zmk-corne-config
python3 tools/nicview_frames.py \
  --manifest tools/animations.toml \
  --output boards/shields/claude_view/assets/claude_art.c
```

Commit the updated `claude_art.c` when you want CI / other clones to build the same pixels.

---

## Manifest reference (`tools/animations.toml`)

Each animation is one **`[[animations]]`** block.

| Key | Required | Meaning |
|-----|----------|---------|
| `name` | yes | Short id used in C symbol prefixes (e.g. `look` → `look_1`, `look_frames`). |
| `dir` | yes | Directory of frame images **relative to cwd** (repo root recommended). |
| `ms_per_frame` | no | Frame timing; default comes from CLI (`--ms-per-frame`, default 200) if omitted. |
| `sequence` | no | Comma-separated **1-based** frame indices, e.g. `1,2,3,2`. Overrides `ping_pong` if both are set. |
| `ping_pong` | no | If `true` and `sequence` is omitted: play **1→N** then **N−1→2** (no snap from last frame back to first). |
| `fuzz`, `padding`, `dither`, `threshold` | no | Override ImageMagick / pipeline options per animation (see `tools/nicview_frames.py` `load_manifest`). |
| `no_rotate` | no | If true, skips the default 90° CW rotation (only if your hardware layout differs). |

Supported image extensions: **jpg, jpeg, png, bmp, gif, webp** (non-dotfiles, sorted by name).

**TOML cannot “comment out” a table with one `#` line** — comment **every** line of the block (including `# [[animations]]` and the keys), or remove the block. Keep a short note above the commented block so future you (or an agent) knows how to re-enable (see the **boo** example in the current manifest).

---

## Checklist: add a new animation

1. Add a folder of frames under the repo (or ensure an existing path is correct). Track it in git **unless** it is intentionally local-only; add to **`.gitignore`** only if frames must not be published.
2. Append a new **`[[animations]]`** block at the desired position in **`tools/animations.toml`** (order = index).
3. Run **`nicview_frames.py`** as above; confirm stderr lists the new animation and `animation_count` in the generated `.c` matches expectations.
4. In **`config/eyelash_corne.keymap`**, under `behaviors`:
   - Add a **`zmk,behavior-anim-set`** node with **`animation-index = <N>`** where **N** is the new animation’s index.
   - **Behavior node label** (the `foo:` in `foo: foo {`) is what ZMK uses for BLE “run behavior” naming on splits. Keep the **label ≤ 9 characters** so the name fits split constraints (this repo documents that on `anim_party` / `anim_cyc`).
5. Bind **`&your_behavior`** on the keymap layer you want (e.g. Fn layer).
6. Update **`INITIAL_ANIM`** comment in **`anim_select.c`** if you change boot semantics; change **`#define INITIAL_ANIM`** only if boot should start on a non-zero index.
7. Update **`boards/shields/claude_view/custom_status_screen.c`** file header comment if it lists keys/animations.
8. Optionally refresh **`keymap-drawer/`** (`eyelash_corne.yaml` / `.svg`) so the diagram matches the keymap.

---

## Checklist: temporarily remove or skip an animation

Goal: **smaller firmware / faster UF2** or fewer choices, **without** deleting the frame source from disk long-term.

1. In **`tools/animations.toml`**, comment out the entire **`[[animations]]`** block (line-comment each line). Add a one-line note: re-enable steps (uncomment → regen → fix keymap indices).
2. Regenerate **`claude_art.c`** — the dropped animation’s pixels are no longer linked into the image.
3. **Renumber** every remaining **`zmk,behavior-anim-set`** `animation-index` to match the new manifest order.
4. **Comment out** unused behavior nodes in the keymap (or delete them) and replace **`&anim_…`** bindings with **`&trans`** or another behavior so the tree still parses.
5. Update comments in **`anim_select.c`**, **`custom_status_screen.c`**, and **`animations.toml`** header so indices and key names stay truthful.
6. Refresh **keymap-drawer** artifacts if you care about diagram accuracy.

---

## Split keyboard / BLE notes

- **`zmk,behavior-anim-set`** and **`zmk,behavior-anim-cycle`** use **`BEHAVIOR_LOCALITY_GLOBAL`** so the peripheral half updates when the central invokes the behavior (same idea as RGB underglow).
- Prefer **zero-parameter** behaviors (`#binding-cells = <0>`) for animation switching on split BLE; parametric “pick index” behaviors are brittle across the link.
- **Short behavior labels** (≤ 9 chars) avoid split run-behavior name limits — see comments on **`anim_party`** and **`anim_cyc`** in the keymap.

---

## Files you will touch most often

| Path | Role |
|------|------|
| `tools/animations.toml` | Source of truth for **which** animations exist and **order** (indices). |
| `tools/nicview_frames.py` | Generator; read `--help` and module docstring for flags. |
| `boards/shields/claude_view/assets/claude_art.c` | **Generated** — do not hand-edit; regen from manifest. |
| `boards/shields/claude_view/assets/claude_art.h` | Stable struct / extern declarations (hand-maintained). |
| `config/eyelash_corne.keymap` | `animation-index` and key bindings. |
| `boards/shields/claude_view/behaviors/anim_select.c` | Boot index `INITIAL_ANIM`. |
| `behavior_anim_set.c` / `behavior_anim_cycle.c` | Behavior implementations (rarely need changes for new art). |
| `dts/bindings/zmk,behavior-anim-set.yaml` | Devicetree binding docs for `animation-index`. |

---

## Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| Wrong animation when pressing a key | **`animation-index`** in keymap does not match **current** manifest order after add/remove/reorder. Regenerate `claude_art.c` after manifest edits. |
| Build fails or generator errors on `dir` | Path wrong (run from repo root) or directory missing / empty / no supported images. |
| `anim_cyc` skips or feels wrong | `animation_count` in generated `.c` does not match how many entries you think are active — fix manifest and regen. |
| UF2 copy flaky or very slow | **`claude_art.c`** is huge with many frames; dropping an animation or reducing frames shrinks the binary. |

---

## Agent summary (one paragraph)

**Manifest order defines 0-based indices.** After any change to **`tools/animations.toml`**, run **`nicview_frames.py`** to refresh **`claude_art.c`**, then align **`animation-index`** on every **`zmk,behavior-anim-set`** in **`config/eyelash_corne.keymap`** with the new order, update **`INITIAL_ANIM`** / header comments if needed, and adjust key bindings or keymap-drawer. Use **commented-out** `[[animations]]` blocks and commented behavior nodes to keep optional animations in-repo without shipping them in firmware.
