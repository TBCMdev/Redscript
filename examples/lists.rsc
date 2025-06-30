use lang;

const x: int = 4;

::msg(@r, x);

mylist: int = [1, 2, 3, x];

lists::append(mylist, 44);
// lists::extend([12, "testing"]);

::msg(@r, mylist);
