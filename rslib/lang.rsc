
// object pair
// {
//     required i1: any;
//     required i2: any;

//     __compare__(i1, i2);
// }

/*
gets an attribute attached to a value in storage.
this function only works on variables and objects.
*/
method<_Type>: void getattr (const __v: _Type, __attr: string)             __cpp__;
/*
sets an attribute attached to a value in storage.
this function only works on variables and objects.
if the attribute doesn't exist, it is created.
you can use this to attach extra data to your variables.
*/
method<_Type, _Val>: void setattr (const __v: _Type, __attr: string!, __val: _Val)  __cpp__;

method<_Type>: void msg  (__p: selector!, __msg: _Type?)      __cpp__;
method:    void kill     (__p: selector!)                     __cpp__;

/*
returns the type of a variable/object attribute.
NOTE: as of redscript alpha, only use this function with something
stored in storage.
*/
// method: int! type    (__v: any)             __single__, __nocompile__
// { return getattr(__v, "type"); }
method<_Type>: int type    (const __v: _Type)                               __cpp__;

/*
Houses all helper functions to do with lists.
Some of these functions are here instead of being inbuilt
into the language as they are more computationally expensive
such as array::at. Use these functions wisely!
*/
module array
{
    /*
    returns the element located at the index __index inside __l.
    You can use this function to access lists via non-constant like values,
    however it is more expensive to compute, as it uses macros.
    */
    method<_ElType>: _ElType at(const __l: _ElType[], const __index: int) extern;

    method<_ElType>: _ElType[] resize(const __l: _ElType[], const begin: int, const end: int) extern;
}