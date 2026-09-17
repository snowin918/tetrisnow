# Gameplay character art

`BattleActors.png` is the active transparent atlas, generated with the built-in imagegen tool. Older emotion sheets are retained as references and are no longer loaded.

Four columns and four rows: boy in the first eight cells, girl in the last eight. Each set is ordered: ready, windup, release, recovery, hurt, frozen, victory, defeat. Both face right in the atlas; the renderer mirrors the girl. These are key poses with procedural breathing, weight shifts and hops, rather than an eight-frame loop per reaction.

Individual reaction clocks start at each event. Line clears trigger throws, with a 0.18-second visual windup before projectiles appear. Impacts trigger recoil. Match results override freezing and persist until reset.

## Generation prompt

Use case: stylized-concept. Create a production game sprite atlas for a winter snowball battle game, PNG with genuinely transparent background. Square 2048x2048 canvas, EXACTLY 4 columns and 4 rows of equal 512x512 cells, no grid lines, no text. Each cell contains one full-body character, including boots, centered horizontally at the same scale, foot baseline at 90% cell height, generous clear margins on all sides. Never let a sprite cross its cell boundaries. Clean polished hand-painted 2D game illustration, crisp simple silhouettes readable at 120px, soft cel shading, restrained detail, expressive faces, consistent anatomy and clothes across all poses. Three-quarter view facing RIGHT for ALL sprites. No background, no ground, no shadows, no lettering, no decorative particles or motion streaks.
Boy: youthful silver-haired boy, blue winter parka, navy trousers, blue scarf, mittens, sturdy snow boots.
Girl: youthful magenta-haired girl with ponytail, ivory winter parka with berry pink trim, teal scarf, navy trousers, mittens, snow boots.
Rows 1 and 2 are the SAME BOY, eight poses in reading order: relaxed ready stance; windup with snowball hand drawn back; snowball throwing follow-through with empty forward hand; relaxed recovery stance; recoil from snow hit arms defensive; cold shiver arms hugging chest; happy victory arms raised; disappointed defeat kneeling.
Rows 3 and 4 are the SAME GIRL, exact same eight poses in reading order: ready; snowball windup; throwing follow-through; recovery; hurt recoil; shivering; victory; kneeling defeat.
Keep each character's face, hair, clothing and body proportions identical across poses. Consistent fixed camera and scale. These are animation key poses, not unrelated portraits. Exactly 16 sprites.

