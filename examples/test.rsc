use lang;
x: int = 4;
const z: int = 2;
x = 4 + 4 + 4 + 4 + z;
lis: int[] = [];

x = lis[4];

// todo fix generic error, error in at func above, and add macro support where needed 
lis[0] = 2;
x = array::at(lis, 1);
msg(@r, x); 