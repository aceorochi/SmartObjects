namespace js
{
  enum retain_policy
  {
    retain,
    copy,
    assign
  };
  
  template <class T, retain_policy Rp>
  struct autoprop;
  
  template <class T, retain_policy Rp>
  struct assignment_traits
  { 
    static void set(autoprop<T,Rp> &dest, T *value);
    static void cleanup(T *val);
  };
  
  template <class T, retain_policy Rp = retain>
  struct autoprop
  {
    autoprop() { }
    autoprop(T *val) : _ptr(val) { }
    
    ~autoprop()
    {
      assignment_traits<T,Rp>::cleanup(_ptr);
      _ptr = nil;
    }
    
    autoprop<T>& operator =(T* val)
    {
      assignment_traits<T,Rp>::set(_ptr,val);
      return *this;
    }    
    
    operator T() const { return _ptr; }
    id operator()() const { return _ptr; }

    friend void assignment_traits<T,Rp>::set(T*&, T*);
    friend void assignment_traits<T,Rp>::cleanup(T*);
    
  private:
    T* _ptr;
  };
  
  
  
  template <class T>
  struct assignment_traits<T, retain>
  {
    static void set(T *&dest, T *value)
    {
      [dest autorelease];
      dest = [value retain];
    }
    
    static void cleanup(T *val)
    {
      [val release];
    }
  };
  
  template <class T>
  struct assignment_traits<T, copy>
  {
    static void set(T *&dest, T *value)
    {
      [dest autorelease]; 
      dest = [value copy];
    }
    
    static void cleanup(T *val)
    {
      [val release];
    }
  };
  
  template <class T>
  struct assignment_traits<T, assign>
  {
    static void set(T *&dest, T *value)
    {
      dest = value;
    }
    
    static void cleanup(T *val)
    {
    }
  };
}