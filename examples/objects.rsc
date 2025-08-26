use lang;

object X
{
required:
    y: int;
    z: int;

optional:
    lifetime: X;

seperate:
    sep: X;   
}
_xtest: X = new (111, 222);
test: X = new(1, 4, _xtest);
test2: X[] = [new X(1, z=4)];

test2[0].y = 4;

msg(@r, test2[0].y);


object bst
{

    value: int;

    lhs: pointer<bst>;
    rhs: pointer<bst>;
}

object<T> pointer
{
    val: T?;
}


new y(new y(null));

y.z = new