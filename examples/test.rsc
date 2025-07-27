x:int;
y:int;
z:int;

w: int = x;

method: void stuff(param: int)
{
    copyParam: int& = param;
}

stuff(x);

var: int = 12;