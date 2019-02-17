#ifndef __STEVEDORE_H__
#define __STEVEDORE_H__

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;
using std::pair;

/* unload */
class S {
private:
  char *c;
  char *b;

public:
  S(char *r) : c(r), b(r) {
  }

  int finish() {
    return c - b;
  }

  template<typename T>
  S* s(const T *t) {
    return this;
  }

  template<typename T>
  S* s_array(const T *begin, int n) {
    int i;
    for (i = 0; i < n; i++) {
      s(begin + i);
    }
    return this;
  }

  template<typename T>
  S* s_vector(const vector<T> *v) {
    int i;
    int len = v->size();
    s<int>(&len);
    for (i = 0; i < len; i++) {
      s(&(v->at(i)));
    }
    return this;
  }

  template<typename K, typename V>
  S* s_map(const map<K, V> *m) {
    int size = m->size();
    s(&size);
    auto it = m->begin(), end = m->end();
    for ( ; it != end; it++) {
      s(&(it->first));
      s(&(it->second));
    }
    return this;
  }

}; /* class S */

template<>
S* S::s(const char *d) {
  *(c++) = *(d);
  return this;
}

template<>
S* S::s(const int *d) {
  memcpy(c, d, 4);
  c += 4;
  return this;
}

template<>
S* S::s(const string *str) {
  int len = str->size();
  s<int>(&len);
  memcpy(c, str->c_str(), len);
  c += len;
  return this;
}

template<>
S* S::s_array(const char *begin, int n) {
  memcpy(c, begin, n);
  c += n;
  return this;
}


/* load */
class P {
private:
  char *c;
  char *b;

public:
  P(char *r) : c(r), b(r) {
  }

  int finish() {
    return c - b;
  }

  template<typename T>
  P* p(T *t) {
    return this;
  }

  template<typename T>
  P* p_array(T *begin, int n) {
    int i;
    for (i = 0; i < n; i++) {
      p(begin + i);
    }
    return this;
  }

  template<typename T>
  P* p_vector(vector<T> *v) {
    int i;
    int size;
    p(&size);
    for (i = 0; i < size; i++) {
      T t;
      p(&t);
      v->push_back(t);
    }
    return this;
  }

  template<typename K, typename V>
  P* p_map(map<K, V> *m) {
    int i;
    int size;
    p(&size);
    for (i = 0; i < size; i++) {
      K k;
      p(&k);
      V v;
      p(&v);
      m->insert(std::pair<K, V>(k, v));
    }
    return this;
  }
}; /* class P */

template<>
P* P::p(char *s) {
  *s = *c;
  c++;
  return this;
}

template<>
P* P::p(int *d) {
  memcpy(d, c, 4);
  c += 4;
  return this;
}

template<>
P* P::p(string *s) {
  int len;
  p(&len);
  s->append(c, len);
  c += len;
  return this;
}

template<>
P* P::p_array(char *begin, int n) {
  memcpy(begin, c, n);
  c += n;
  return this;
}


static int test() {
  char buf[1024];
  int uid = 10086;
  string name = "apple";
  int year[3] = {2015, 2016, 2017};
  vector<int> v = {10, 90, 180, 250, 260};
  map<int, int> m;
  m[201] = 10;
  m[202] = 15;
  m[203] = 19;
  auto sd  = S(buf);
  sd.s(&uid)->s(&name)->s_array(year, 3)->s_vector(&v)->s_map(&m);

  int r_uid;
  string r_name;
  int r_year[3];
  vector<int> r_v;
  map<int, int> r_m;

  auto ds = P(buf);
  ds.p(&r_uid)->p(&r_name)->p_array(r_year, 3)->p_vector(&r_v)->p_map(&r_m);

  printf("r_uid is %d\n", r_uid);
  printf("r_name is %s\n", r_name.c_str());
  for (int i = 0;  i < 3; i++) {
    printf("r_year[%d] is %d\n", i, r_year[i]);
  }
  for (int i = 0; i < r_v.size(); i++) {
    printf("r_v[%d] is %d\n", i, r_v[i]);
  }
  for (auto it = r_m.begin(); it != r_m.end(); it++) {
    printf("r_m[%d] is %d\n", it->first, it->second);
  }
  return 0;
}


#endif /* __STEVEDORE_H__ */
