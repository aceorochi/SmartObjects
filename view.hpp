namespace js
{
  template <class K = id>
  struct view
  {
    struct assignable
    {
      id reference;
      K key;
      
      id& operator =(id val) { set_value(key,val); return reference; }
      operator id() const { return value_at(key); }
      id operator ()() const { return value_at(key); }
      
      id value_at(const id key) const { return [reference valueForKey:key]; }
      id value_at(const int i) const { return [reference objectAtIndex:i]; }
      void set_value(const id key, id val) { [reference setValue:val forKey:key]; }
    };
    
    view(id val) : reference(val) {}
    assignable operator[](K key)
    {
      return (assignable){ reference, key };
    }
    
    operator id() const { return reference; }
    id operator()() const { return reference; }
    
  private:
    id reference;
  };
}