#import <map>

namespace js
{
  enum retain_policy
  {
    retain,
    copy,
    assign
  };
  
  // ** BE VERY CAREFUL. THERE ARE SOME MEMORY PROBLEMS HERE. **
    
  // `autoprop` is a thin wrapper for pointers to Objective-C objects, to be
  // used in the generation of properties. Take the following example:
  //
  //     @interface Dog : NSObject
  //     @property (copy) NSString *name;
  //     @end
  //
  //     @implementation Dog
  //     @synthesize name;
  //
  //     - (void)dealloc {
  //       [name release];
  //       [super dealloc];
  //     }
  //
  //     @end
  //
  // When `name` is synthesized, an instance variable `name` is generated, and
  // two methods (`-setName:` and `-name`) are added onto `Dog`. `-setName:`
  // will release the current `name`, copy the new one and assign it to the
  // instance variable. The assignment semantics provided with the `@property`
  // directive (`retain`, `copy` or `assign`) affect the way these are
  // implemented. Now consider the following:
  // 
  //     @interface Dog : NSObject
  //     @property js::autoprop<NSString,js::copy> name;
  //     @end
  //
  //     @implementation Dog
  //     @synthesize name;
  //     @end
  //
  // There are a few differences:
  //
  // 1. The type of `name` is no longer `NSString*`, but
  // `js::autoprop<NSString,js::copy>`.
  // 2. The property is actually `assign` (that is, the Objective-C Runtime
  // generates a setter with `assign`-semantics); `autoprop`, however, handles
  // assignment internally using `copy`-semantics (as specificed by `js::copy`).
  // 3. We didn't have to release `name` in `-dealloc`. *This is a big fucking
  // deal.*
  //
  // The reason we don't have to clean up after ourselves is that Objective-C
  // actually calls available destructors for C++ object instance variables
  // in `-dealloc`. In our destructor, we release the referenced object (unless
  // `js::assign` is specified).
  template <class T, retain_policy Rp = retain>
  struct autoprop
  {
    autoprop() { }
    autoprop(T* value)
    {
      assign_value(value);
    }
    
    ~autoprop()
    {
      cleanup_value();
    }
    
    autoprop<T>& operator =(T* val)
    {
      if (Rp != assign) { [_ptr autorelease]; }
      assign_value(_ptr,val,Rp);      
      return* this;
    }
    
    operator id() const { return _ptr; }
    T* operator()() const { return _ptr; }    

  private:
    void assign_value(id value)
    {
      if (Rp == retain) { _ptr = [value retain]; }
      else if (Rp == copy) { _ptr = [value copy]; }
      else { _ptr = value; }
    }
    
    void cleanup_value()
    {
      if (Rp != assign) { [_ptr release]; }
    }
    
    T* _ptr;
  };
  
}