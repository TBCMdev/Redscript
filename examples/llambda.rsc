use lang;

a: int = 5;

method: void b() {
    a = 2;
}

method: void c() {
    a: int = 0;
    b();
    msg(@r, a);
}

c();