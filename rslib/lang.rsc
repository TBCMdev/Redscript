/*
Houses the imporant language functions of the redscript language.

@author TBCMDev

Beta note: AVOID_ERROR_CHECK is a temporary generic type used to delay function compilation.
           any function with that as a template should not be used.
*/
/*
gets an attribute attached to a value in storage.
this function only works on variables and objects.
*/
method<_Type>: void getattr (const __v: _Type&, __attr: string)             __cpp__;
/*
sets an attribute attached to a value in storage.
this function only works on variables and objects.
if the attribute doesn't exist, it is created.
you can use this to attach extra data to your variables.
*/
method<_Type, _Val>: void setattr (const __v: _Type&, __attr: string, __val: _Val&)  __cpp__;

/*
Sends a message __msg to the entity selector __p.

Note that in redscript alpha, printing a list will look different to what it looks like in code.
See the docs for more info.
*/
method<_Type>:   void msg  (__p: selector, __msg: _Type?)      __cpp__;
method:          void kill (__p: selector)                     __cpp__;

method<_ElType>: int len   (const __l: _ElType[]?)               __cpp__;
method:          int slen  (const __str: string)                __cpp__;

/*
Houses all helper functions to do with lists.
Some of these functions are here instead of being inbuilt
into the language as they are more computationally expensive
such as array::at. Use these functions wisely!

THESE FUNCTIONS ARE NOT IMPLEMENTED. You can call them and they will do nothing.
*/
module array
{
    /*
    returns the element located at the index __index inside __l.
    You can use this function to access lists via non-constant like values,
    however it is more expensive to compute, as it uses macros.

    If __index is not within the bounds of the array, null is returned.

    */
    method<_ElType>: _ElType?  at     (const __l: _ElType[], __index: int) extern;

    /*
    Appends __value to the end of __l.
    */
    method<_ElType>: void      add    (const __l: _ElType[]&, const __value: _ElType) extern;

    /*
    Removes the element inside __l at the index __at.
    */
    method<_ElType>: void      remove (const __l: _ElType[]&, __at: int) extern;

    /*
    Copies and resizes the array __l based on the new bounds begin and end.
    
    If begin | end are not valid indicies, an empty array is returned.
    */
    method<_ElType>: _ElType[] resize (const __l: _ElType[]&, begin: int, end: int) extern;

    /*
    Removes all elements from __l.
    */
    method<_ElType>: void      clear  (const __l: _ElType[]&) extern;

    /*
    Gets the index of the first value inside __l to match __val.
    If _ElType (the type of the lists elements) does not have a comparison operator defined, a compile_error is thrown.

    For all primitive types, this is not a problem.
    */
    method<_ElType>: int       index  (const __l: _ElType[]&, const __val: _ElType) extern wrapper __nocompile__;
}
    /*
    Prints a message to the person who ran the command.

    Wraps msg() impl.

    
    */
    // method: int stuff (message: string)
    // { msg(@r, "message"); return 1; }