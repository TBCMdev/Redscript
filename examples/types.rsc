
method: void y()
{
    z: selector = @p;
    test: int = 12 + 44;
    x: int[][] = [[test,test,3], [4,5,6]];
    // test is appeneded to x[0], not x[0][0].
    // todo fix!
}