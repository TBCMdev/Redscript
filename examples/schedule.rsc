use lang;

method: void y()
{
    method: void z()
    {
        wait(4);
    }
    x: int = 4;
    schedule(z, 1, method: void ()
    {
        x = 10;
    })
}
