use lang;

const x: int = 4;

::msg(@r, x);

mylist: int[] = [1, 2, 3, x];

stuffX: int = mylist[2];
// lists::append(mylist, 44);
// lists::extend([12, "testing"]);

method<T>: T[][] printlist(l: T[])
{
    // doesnt work needs impl
    // if (l[0] == 2)
    // {
    //     msg(@r, l);
    // }
    ret = [l];
    return ret;
}
printlist<int>(mylist);
// BUG: I can reference y in generic functions as it hasnt been instantiated until after y is defined.
// not fatal to the program, just bad practice.
const y: int = mylist[0];
// no error given!


const el: int = ::array::at<int>(y, 2);

myList: string[] = ["Hello", "World!"];

array::add<int>(mylist, 4);

// wont work, need to fix tellraw impl
// ::msg(@r, mylist[0]);

