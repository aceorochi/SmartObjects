#import <objc/runtime.h>
#import <set>
#import <memory>

namespace js
{  
  enum ownership_policy
  {
    not_owning,
    release
  };

  // `dynamic_subclass` contains the implementations that will be used whenever
  // a subclass is generated for a referenced object. This struct template takes
  // type parameters: a class `T` which is the type of the referenced object,
  // and a class `CallbackTrait` which implements:
  //
  //     static void clear_refs(T*);
  //
  //
  template <class T, class CallbackTrait>
  struct dynamic_subclass;
  
  template <class T, ownership_policy Op = not_owning>
  struct ref
  {
    ref(T* object) : _ptr(object)
    {
      object_setClass(object, get_dynamic_subclass());
      _refs().insert(object);
    }
    
    ~ref()
    {
      if (Op == release) { [_ptr release]; }
    }
    
    T* target() const
    {
      return _refs().count(_ptr) > 0 ? _ptr : nil;
    }
     
    operator T*() const
    {
      return target();
    }
    
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
        
        typedef dynamic_subclass<T,ref<T,Op> > DynamicImp;
        setup_subclass_method(subclass,@selector(dealloc),DynamicImp::dealloc_imp);
        setup_subclass_method(subclass,@selector(class),DynamicImp::class_imp);
        
        objc_registerClassPair(subclass);
      }
      
      return subclass;
    }
    
    template <class Sig>
    static void setup_subclass_method(Class subclass, SEL sel, Sig imp)
    {
      Class superclass = class_getSuperclass(subclass);
      Method m = class_getInstanceMethod(superclass, sel);
      class_addMethod(subclass, sel, reinterpret_cast<IMP>(imp), method_getTypeEncoding(m));
    }
    
    
    static std::set<T*>& _refs()
    {
      static std::set<T*> _refs;
      return _refs;
    }
    
    T* _ptr;
  };
  
  
  template <class T, class CallbackTrait>
  struct dynamic_subclass 
  {
    static void dealloc_imp(T* self, SEL s)
    {
      CallbackTrait::clear_refs(self);
      original_implementation(self,s);
    }
    
    static Class class_imp(T* self, SEL s)
    {
      return original_superclass(self);
    }
    
    
  private:
    static Class original_superclass(T* self)
    {
      return class_getSuperclass([self class]);
    }
    
    static IMP original_implementation(T* self, SEL sel)
    {
      return class_getMethodImplementation(original_superclass(self), sel);
    }
  };  
}
