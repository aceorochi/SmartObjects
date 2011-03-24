namespace js
{
  template <class K = id>
  struct view
  {
    typedef K(^KeyTransformer)(K);
    typedef id(^ValueTransformer)(id);
    
    view(id val) : ptr(val), ktr(nil), vtr(nil) { }
    view(view<K>& other) : ptr(other.ptr), ktr(nil), vtr(nil) { };
    view(id val, KeyTransformer blk) : ptr(val), ktr([blk copy]), vtr(nil) { }
    view(id val, KeyTransformer kblk, ValueTransformer vblk) : ptr(val), ktr([kblk copy]), vtr([vblk copy]) { }
    
    ~view()
    {
      [ktr release]; 
      [vtr release];
    }
    
    struct assignable;
    assignable operator[](K key)
    {
      K real_key = ktr ? ktr(key) : key;
      return (assignable){ ptr, real_key };
    }
    
    operator id() const { return ptr; }
    id operator()() const { return ptr; }
    
    struct assignable
    {
      id receiver;
      K key;
      
      id& operator =(id val) { set_value(key,val); return receiver; }
      operator id() const { return value_at(key); }
      id operator ()() const { return value_at(key); }
      
      id value_at(const id key) const { return [receiver valueForKey:key]; }
      id value_at(const int i) const { return [receiver objectAtIndex:i]; }
      void set_value(const id key, id val) { [receiver setValue:val forKey:key]; }
    };
    
    
  private:
    id ptr;
    KeyTransformer ktr;
    ValueTransformer vtr;
  };
}