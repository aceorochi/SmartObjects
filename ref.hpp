#import <objc/runtime.h>
#import <set>
#import <memory>

namespace js {
  template <class T>
  struct dynamic_subclass;
  
  enum ownership_policy {
    not_owning,
    release
  };
  
  template <class T, ownership_policy Op = not_owning>
  struct ref
  {  
    ref(T *object) : _ptr(object)
    {
      object_setClass(object, get_dynamic_subclass());
      _refs().insert(object);
    }
    
    ~ref() {
      if (Op == not_owning) {
        [_ptr release];
      }
    }
    
    T *target() const
    {
      return exists() ? _ptr : nil;
    }
    
    
    operator T*() const
    {
      return target();
    }
    
    bool exists() const
    {
      return _refs().count(_ptr) > 0;
    }
    
    static void clear_refs(T *object)
    {
      _refs().erase(object);
    }
    
  private:
    Class get_dynamic_subclass()
    {
      Class original = [_ptr class];
      
      NSString *name = [NSString stringWithFormat:@"%@_refReferenceSubclass", original];
      NSLog(@"name: %@", name);
      Class subclass = NSClassFromString(name);
      
      if (subclass == nil) {
        subclass = objc_allocateClassPair(original, [name UTF8String], 0);
        
        setup_subclass_method(subclass,@selector(dealloc),dynamic_subclass<T>::dealloc_imp);
        setup_subclass_method(subclass,@selector(class),dynamic_subclass<T>::class_imp);
        
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
    
    T *_ptr;
    
    static std::set<T*> &_refs()
    {
      static std::set<T*> _refs;
      return _refs;
    }
    
  };
  
  
  template <class T>
  struct dynamic_subclass 
  {
    static void dealloc_imp(T *self, SEL s)
    {
      ref<T>::clear_refs(self);
      real_imp(self,s);
    }
    
    static Class class_imp(T *self, SEL s)
    {
      return real_superclass(self);
    }
    
    
  private:
    static Class real_superclass(T *self)
    {
      return class_getSuperclass(object_getClass(self));
    }
    
    static IMP real_imp(T *self, SEL sel)
    {
      return class_getMethodImplementation(real_superclass(self), sel);
    }
  };  
}
