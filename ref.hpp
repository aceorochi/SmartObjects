#import <objc/runtime.h>
#import <set>

namespace js
{
  // Smart references have an `release_policy Rp`, which is either
  // `strong` (default) or `weak`. *(these names should be improved)*
  // 
  //     ref<NSObject,weak> obj1 = [[NSObject new] autorelease];
  //
  // will not be overreleased. However,
  //
  //     ref<NSObject> obj2 = [NSObject new];
  //
  // will be released when it goes out of scope! This is helpful for getting
  // around autorelease pools, etc.
  enum reference_policy
  {
    strong, // implicit release
    weak // no implicit release
  };
  
  // `dynamic_subclass` contains the implementations that will be used whenever
  // a subclass is generated for a referenced object. This struct template takes
  // type parameters: a class `T` which is the type of the referenced object,
  // and a class `CallbackTrait` which implements:
  //
  //     static void clear_refs(T*);
  //
  template <class T, class CallbackTrait>
  struct subclass_impl;
  
  template <class T = NSObject, reference_policy Rp = strong>
  struct ref
  {
    // When a `ref` is created from an object, it is added to a global set which
    // keeps track of live references; a subclass of the object's class is
    // dynamically generated, and the object has its `isa` pointer switched to
    // the new subclass. This allows us to have some hooks in `-dealloc`, to
    // update the reference-tracker. That way, when referenced objects are
    // deallocated, the our reference is zeroed out.
    ref(T* object) : _ptr(object)
    {
      object_setClass(object, get_dynamic_subclass());
      _refs().insert(object);
    }
    
    ~ref()
    {
      if (Rp == strong) { [_ptr release]; }
    }
    
    // We have three ways to access the referenced object. The first (and
    // preferred method) is the implicit conversion operator, which lets us do
    // the following:
    //
    //     ref<NSString> str = @"asdfasdf";
    //     const char* c_str = [str UTF8String];
    //
    operator T*() const
    {
      return target();
    }
    
    // The second way uses the call-operator; this is kind of nasty, but is
    // useful when we want to use dot-notation for property access on our
    // referenced object.
    // 
    //     ref<NSString> str = @"adfasdf";
    //     const char* fail = str.UTF8String; // results in compile-error
    //     const char* c_str = str().UTF8String; // works as expected
    //
    T* operator ()() const
    {
      return target();
    }
    
    // Sometimes, however, it may be desirable to explicitly get the referenced
    // object. In either case, the referenced object will be returned if it
    // still exists; otherwise, it will register `nil`. This is especially
    // helpful for when we want a weak reference (to avoid retain-cycles):
    //
    //     ref<SomeType,weak> rself = self;
    //     [something doSomethingCopyingBlock:^{
    //       [rself doSomethingElse];
    //       rself.property = YES; // the compiler will fail at casting here.
    //       rself().property = YES;
    //       // operator() helps the compiler with dot-syntax, and is the same
    //       // as using ref::target:
    //       rself.target().property = YES;
    //     }];
    //
    // Either way, we escape block retain-cycles in a healthy way: the `ref` is copied
    // into the block's internal state; since the block doesn't know anything
    // about its contents, the underlying object is not retained. Then, when we
    // use the reference in any way that causes it to be casted to `id` (or
    // otherwise converted), we receive the original object pointer.
    T* target() const
    {
      return _refs().count(_ptr) > 0 ? _ptr : nil;
    }
    
    // When a referenced object is deallocated, it needs to be removed from our
    // global references set.
    static void clear_refs(T* object)
    {
      _refs().erase(object);
    }
    
  private:
    Class get_dynamic_subclass()
    {
      Class original = [_ptr class];
      
      id name = [NSString stringWithFormat:@"%@_refReferenceSubclass", original];
      Class subclass = NSClassFromString(name);
      
      if (subclass == nil)
      {
        subclass = objc_allocateClassPair(original, [name UTF8String], 0);
        
        typedef subclass_impl<T,ref<T,Rp> > DynamicImp;
        override_method(subclass, @selector(dealloc), DynamicImp::dealloc_imp);
        override_method(subclass, @selector(class), DynamicImp::class_imp);
        
        objc_registerClassPair(subclass);
      }
      
      return subclass;
    }
    
    template <class Sig>
    static void override_method(Class subclass, SEL sel, Sig imp)
    {
      Class superclass = class_getSuperclass(subclass);
      Method m = class_getInstanceMethod(superclass, sel);
      class_addMethod(subclass, sel, reinterpret_cast<IMP>(imp), method_getTypeEncoding(m));
    }
    
    
    // A static `std::set<T*> _refs` is lazily initialized and accessed through
    // `_refs()`; this keeps track of all live references. When a `ref` is
    // created, its target is added to `_refs`; when that target is deallocated,
    // then it is removed from `_refs`.
    static std::set<T*>& _refs()
    {
      static std::set<T*> _refs;
      return _refs;
    }
    
    // Our target object is referenced in `_ptr`; this member is never referred
    // except in `target()`, where it is first determined to be living, and then
    // returned.
    T* _ptr;
  };
  
  
  template <class T, class CallbackTrait>
  struct subclass_impl 
  {
    static void dealloc_imp(T* self, SEL s)
    {
      CallbackTrait::clear_refs(self);
      original_implementation(self,s);
    }
    
    static Class class_imp(T* self, SEL s)
    {
      return original_class(self);
    }
    
    
  private:
    static Class original_class(T* self)
    {
      return class_getSuperclass(object_getClass(self));
    }
    
    static IMP original_implementation(T* self, SEL sel)
    {
      return class_getMethodImplementation(original_class(self), sel);
    }
  };
  
  template <class T>
  struct weak_ref : public ref<T,weak>
  {
    weak_ref(T* object) : ref<T,weak>(object)
    {
    }
  };
}
