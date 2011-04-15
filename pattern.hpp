#import <map>

namespace js
{
  template <class K, class V>
  struct pattern
  {
    pattern(const V& default_value) : _default(default_value) { }
    
    V& operator[](const K& k)
    {
      return _map.find(k) == _map.end() ? _default : _map[k];
    }
    
  private:
    V _default;
    std::map<K,V> _map;
  };
}