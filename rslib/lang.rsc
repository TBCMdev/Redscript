
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
// method: void getattr (__v: any, __attr: string)             __cpp__;
/*
sets an attribute attached to a value in storage.
this function only works on variables and objects.
if the attribute doesn't exist, it is created.
you can use this to attach extra data to your variables.
*/
// method: void setattr (__v: any, __attr: string!, __val: any)  __cpp__;

method<_Type>: void msg  (__p: selector!, __msg: _Type?)       __cpp__;
method:    void kill     (__p: selector!)                     __cpp__;

// more to come

/*
returns the type of a variable/object attribute.
NOTE: as of redscript alpha, only use this function with something
stored in storage.
*/
// method: int! type    (__v: any)             __single__, __nocompile__
// { return getattr(__v, "type"); }
// method: int! type    (__v: any)                               __cpp__;

