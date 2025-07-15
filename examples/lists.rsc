use lang;

const x: int = 4;

::msg(@r, x);

mylist: int[] = [1, 2, 3, x];

// lists::append(mylist, 44);
// lists::extend([12, "testing"]);

method<T>: T printlist(l: T[])
{
    // doesnt work needs impl
    // if (l[0] == 2)
    // {
    //     msg(@r, l);
    // }
    msg(@r, l);


    return l[0];
}

const y: int = mylist[0];

printlist(mylist);

const el: int = array::at(y, 2);

// wont work, need to fix tellraw impl
// ::msg(@r, mylist[0]);

