use lang;

currentX: float = ~[0];

loc: location = new(~2, ^4, ~-3);
place(BLOCK_DIRT, ~);
// setblock ~ ~ ~ minecraft:air destroy
// data get block ~ ~ ~ x
// or if it was in an execute block: data get entity @r Pos[?]

