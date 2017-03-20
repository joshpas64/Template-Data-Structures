#ifndef HASH_MAP_HPP_
#define HASH_MAP_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"
#include "pair.hpp"

// Submitter jpascasc(Pascascio, Joshua)
namespace ics {


#ifndef undefinedhashdefined
#define undefinedhashdefined
template<class T>
int undefinedhash (const T& a) {return 0;}
#endif /* undefinedhashdefined */

//Instantiate the templated class supplying thash(a): produces a hash value for a.
//If thash is defaulted to undefinedhash in the template, then a constructor must supply chash.
//If both thash and chash are supplied, then they must be the same (by ==) function.
//If neither is supplied, or both are supplied but different, TemplateFunctionError is raised.
//The (unique) non-undefinedhash value supplied by thash/chash is stored in the instance variable hash.
template<class KEY,class T, int (*thash)(const KEY& a) = undefinedhash<KEY>> class HashMap {
  public:
    typedef ics::pair<KEY,T>   Entry;
    typedef int (*hashfunc) (const KEY& a);

    //Destructor/Constructors
    ~HashMap ();

    HashMap          (double the_load_threshold = 1.0, int (*chash)(const KEY& a) = undefinedhash<KEY>);
    explicit HashMap (int initial_bins, double the_load_threshold = 1.0, int (*chash)(const KEY& k) = undefinedhash<KEY>);
    HashMap          (const HashMap<KEY,T,thash>& to_copy, double the_load_threshold = 1.0, int (*chash)(const KEY& a) = undefinedhash<KEY>);
    explicit HashMap (const std::initializer_list<Entry>& il, double the_load_threshold = 1.0, int (*chash)(const KEY& a) = undefinedhash<KEY>);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit HashMap (const Iterable& i, double the_load_threshold = 1.0, int (*chash)(const KEY& a) = undefinedhash<KEY>);


    //Queries
    bool empty      () const;
    int  size       () const;
    bool has_key    (const KEY& key) const;
    bool has_value  (const T& value) const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<


    //Commands
    T    put   (const KEY& key, const T& value);
    T    erase (const KEY& key);
    void clear ();

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    int put_all(const Iterable& i);


    //Operators

    T&       operator [] (const KEY&);
    const T& operator [] (const KEY&) const;
    HashMap<KEY,T,thash>& operator = (const HashMap<KEY,T,thash>& rhs);
    bool operator == (const HashMap<KEY,T,thash>& rhs) const;
    bool operator != (const HashMap<KEY,T,thash>& rhs) const;

    template<class KEY2,class T2, int (*hash2)(const KEY2& a)>
    friend std::ostream& operator << (std::ostream& outs, const HashMap<KEY2,T2,hash2>& m);



  private:
    class LN;

  public:
    class Iterator {
      public:
         typedef pair<int,LN*> Cursor;

        //Private constructor called in begin/end, which are friends of HashMap<T>
        ~Iterator();
        Entry       erase();
        std::string str  () const;
        HashMap<KEY,T,thash>::Iterator& operator ++ ();
        HashMap<KEY,T,thash>::Iterator  operator ++ (int);
        bool operator == (const HashMap<KEY,T,thash>::Iterator& rhs) const;
        bool operator != (const HashMap<KEY,T,thash>::Iterator& rhs) const;
        Entry& operator *  () const;
        Entry* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const HashMap<KEY,T,thash>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator HashMap<KEY,T,thash>::begin () const;
        friend Iterator HashMap<KEY,T,thash>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        Cursor                current; //Bin Index + LN* pointer; stops if LN* == nullptr
        HashMap<KEY,T,thash>* ref_map;
        int                   expected_mod_count;
        bool                  can_erase = true;

        //Helper methods
        void advance_cursors();

        //Called in friends begin/end
        Iterator(HashMap<KEY,T,thash>* iterate_over, bool from_begin);
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    class LN {
    public:
      LN ()                         : next(nullptr){}
      LN (const LN& ln)             : value(ln.value), next(ln.next){}
      LN (Entry v, LN* n = nullptr) : value(v), next(n){}

      Entry value;
      LN*   next;
  };

  int (*hash)(const KEY& k);  //Hashing function used (from template or constructor)
  LN** map      = nullptr;    //Pointer to array of pointers: each bin stores a list with a trailer node
  double load_threshold;      //used/bins <= load_threshold
  int bins      = 1;          //# bins in array (should start >= 1 so hash_compress doesn't divide by 0)
  int used      = 0;          //Cache for number of key->value pairs in the hash table
  int mod_count = 0;          //For sensing concurrent modification


  //Helper methods
  int   hash_compress        (const KEY& key)          const;  //hash function ranged to [0,bins-1]
  LN*   find_key             (const KEY& key) const;           //Returns reference to key's node or nullptr
  LN*   copy_list            (LN*   l)                 const;  //Copy the keys/values in a bin (order irrelevant)
  LN**  copy_hash_table      (LN** ht, int bins)       const;  //Copy the bins/keys/values in ht tree (order in bins irrelevant)

  void  ensure_load_threshold(int new_used);                   //Reallocate if load_factor > load_threshold
  void  delete_hash_table    (LN**& ht, int bins);             //Deallocate all LN in ht (and the ht itself; ht == nullptr)
};





////////////////////////////////////////////////////////////////////////////////
//
//HashMap class and related definitions

//Destructor/Constructors

template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::~HashMap() {
    delete_hash_table(map,bins);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(double the_load_threshold, int (*chash)(const KEY& k))
:hash(thash != (hashfunc)undefinedhash<KEY> ? thash : chash),load_threshold(the_load_threshold)
{
    if(hash == (hashfunc)undefinedhash<KEY>)
        throw ics::TemplateFunctionError("HashMap::default constructor: neither specified");
    if(thash != (hashfunc)undefinedhash<KEY> && chash != (hashfunc)undefinedhash<KEY> && thash != chash)
        throw ics::TemplateFunctionError("HashMap::default constuctor: both specified and different");
    if(load_threshold <= 0)
        load_threshold = 1.0;
    ensure_load_threshold(used);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(int initial_bins, double the_load_threshold, int (*chash)(const KEY& k))
:hash(thash != (hashfunc)undefinedhash<KEY> ? thash : chash), load_threshold(the_load_threshold),bins(initial_bins)
{
    if(hash == (hashfunc)undefinedhash<KEY>)
        throw ics::TemplateFunctionError("HashMap::initial_bins constructor: neither specified");
    if(thash != (hashfunc)undefinedhash<KEY> && chash != (hashfunc)undefinedhash<KEY> && thash != chash)
        throw ics::TemplateFunctionError("HashMap::initial_bins constructor: both specified and different");
    if(load_threshold <= 0)
        load_threshold = 1.0;
    if(bins < 1)
        bins = 1;
    ensure_load_threshold(used);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(const HashMap<KEY,T,thash>& to_copy, double the_load_threshold, int (*chash)(const KEY& a))
:hash(thash != (hashfunc)undefinedhash<KEY> ? thash : chash),load_threshold(the_load_threshold)
{
    if(hash == (hashfunc)undefinedhash<KEY>)
        hash = to_copy.hash;
    if(thash != (hashfunc)undefinedhash<KEY> && chash != (hashfunc)undefinedhash<KEY> && chash != thash )
        throw ics::TemplateFunctionError("HashMap::copy constructor: both specified and different");
    if(load_threshold <= 0)
        load_threshold = to_copy.load_threshold;
    if(hash == to_copy.hash){
        used = to_copy.used;
        bins = to_copy.bins;
        map = copy_hash_table(to_copy.map,bins);
    }
    else{
        for(const Entry& e : to_copy)
            put(e.first,e.second);
    }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::HashMap(const std::initializer_list<Entry>& il, double the_load_threshold, int (*chash)(const KEY& k))
:hash(thash != (hashfunc)undefinedhash<KEY> ? thash : chash),load_threshold(the_load_threshold)
{
    if(hash == (hashfunc)undefinedhash<KEY>)
        throw ics::TemplateFunctionError("HashMap::initializer_list constructor: neither specified");
    if(thash != (hashfunc)undefinedhash<KEY> && chash != (hashfunc)undefinedhash<KEY> && chash != thash)
        throw ics::TemplateFunctionError("HashMap::initializer_list constructor: both specified and different");
    if (load_threshold <= 0)
        load_threshold = 1.0;
    for(const Entry& e: il)
        put(e.first,e.second);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
template <class Iterable>
HashMap<KEY,T,thash>::HashMap(const Iterable& i, double the_load_threshold, int (*chash)(const KEY& k))
:hash(thash != (hashfunc)undefinedhash<KEY> ? thash : chash),load_threshold(the_load_threshold)
{
    if(hash == (hashfunc)undefinedhash<KEY>)
        throw ics::TemplateFunctionError("HashMap::Iterable constructor: neither specified");
    if(thash != (hashfunc)undefinedhash<KEY> && chash != (hashfunc)undefinedhash<KEY> && thash != chash)
        throw ics::TemplateFunctionError("HashMap::Iterable constructor: both specified and different");
    if(load_threshold <= 0)
        load_threshold = 1.0;
    for(const Entry& e : i)
        put(e.first,e.second);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::empty() const {
    return used == 0;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
int HashMap<KEY,T,thash>::size() const {
    return used;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::has_key (const KEY& key) const {
    return find_key(key) != nullptr;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::has_value (const T& value) const {
    if(this->empty())
        return false;
    for(int i = 0; i < bins; i++){
        for(LN* temp = map[i]; temp->next != nullptr; temp = temp->next){
            if(temp->value.second == value)
                return true;
        }
    }
    return false;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
std::string HashMap<KEY,T,thash>::str() const {
    std::ostringstream answer;
    answer << "HashMap[";
    if(used > 0) {
        for (int i = 0; i < bins; i++) {
            answer << "\nbin[" << i << "]:   ";
            for (HashMap::LN *temp = map[i]; temp->next != nullptr; temp = temp->next)
                answer << temp->value.first << "->" << temp->value.second << "->";
            answer << "TRAILER";
        }
    }
    answer << "](used=" << used << ",bins=" << bins << ",mod_count=" << mod_count << ")";
    return answer.str();
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class KEY,class T, int (*thash)(const KEY& a)>
T HashMap<KEY,T,thash>::put(const KEY& key, const T& value) {
    LN* temp = find_key(key);
    ++mod_count;
    if(temp == nullptr){
        ensure_load_threshold(++used);
        int index = hash_compress(key);
        map[index] = new LN(Entry(key,value),map[index]);
        return value;
    }
    T returnVal = temp->value.second;
    temp->value.second = value;
    return returnVal;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
T HashMap<KEY,T,thash>::erase(const KEY& key) {
    LN* temp = find_key(key);
    if(temp != nullptr){
        LN* to_delete = temp->next;
        T returnVal = temp->value.second;
        *temp = *(temp->next);
        delete to_delete;
        --used;
        ++mod_count;
        return returnVal;
    }
    std::ostringstream alt;
    alt << "HashMap::erase:key(" << key << ") not in HashMap";
    throw ics::KeyError(alt.str());
}


template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::clear() {
    delete_hash_table(map,bins);
    ++mod_count;
    used = 0;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
template<class Iterable>
int HashMap<KEY,T,thash>::put_all(const Iterable& i) {
    int count = 0;
    for(const Entry& e : i){
        put(e.first,e.second);
        ++count;
    }
    return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class KEY,class T, int (*thash)(const KEY& a)>
T& HashMap<KEY,T,thash>::operator [] (const KEY& key) {
    HashMap::LN* temp = find_key(key);
    if(temp == nullptr){
        ++mod_count;
        ensure_load_threshold(++used);
        int index = hash_compress(key);
        map[index] = new HashMap::LN(Entry(key,T()),map[index]);
        return map[index]->value.second;
    }
    return temp->value.second;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
const T& HashMap<KEY,T,thash>::operator [] (const KEY& key) const {
    LN* temp = find_key(key);
    if( temp != nullptr)
        return temp->value.second;
    std::ostringstream alt;
    alt << "HashMap::operator []: key(" << key << ") not in map";
    throw ics::KeyError(alt.str());
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>& HashMap<KEY,T,thash>::operator = (const HashMap<KEY,T,thash>& rhs) {
    if(this == &rhs)
        return *this;
    clear();
    if(hash == rhs.hash){
        map = copy_hash_table(rhs.map,rhs.bins);
        used = rhs.used;
        bins = rhs.bins;
    }
    else {
        hash = rhs.hash;
        put_all(rhs);
    }
    ++mod_count;
    return *this;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::operator == (const HashMap<KEY,T,thash>& rhs) const {
    if(this == &rhs)
        return true;
    if(used != rhs.used)
        return false;
    HashMap::LN* temp;
    for(HashMap::Iterator i = rhs.begin(); i != rhs.end(); ++i){
        temp = find_key(i->first);
        if(temp == nullptr || temp->value.second != i->second)
            return false;
    }
    return true;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::operator != (const HashMap<KEY,T,thash>& rhs) const {
    return !(*this == rhs);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
std::ostream& operator << (std::ostream& outs, const HashMap<KEY,T,thash>& m) {
    outs << "map[";
    if(!(m.empty())){
        typename HashMap<KEY,T,thash>::Iterator i = m.begin();
        outs << i->first << "->" << i->second;
        ++i;
        for(; i != m.end(); ++i)
            outs << "," << i->first << "->" << i->second;
    }
    outs << "]";
    return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class KEY,class T, int (*thash)(const KEY& a)>
auto HashMap<KEY,T,thash>::begin () const -> HashMap<KEY,T,thash>::Iterator {
    return Iterator(const_cast<HashMap<KEY,T,thash>*>(this),true);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
auto HashMap<KEY,T,thash>::end () const -> HashMap<KEY,T,thash>::Iterator {
    return Iterator(const_cast<HashMap<KEY,T,thash>*>(this),false);
}


///////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class KEY,class T, int (*thash)(const KEY& a)>
int HashMap<KEY,T,thash>::hash_compress (const KEY& key) const {
    int result = ((hash(key)) % bins);
    if(result < 0)
        result = result * -1;
    return result;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
typename HashMap<KEY,T,thash>::LN* HashMap<KEY,T,thash>::find_key (const KEY& key) const {
    if(map == nullptr)
        return nullptr;
    for(LN* temp = map[hash_compress(key)]; temp->next != nullptr; temp = temp->next){
        if(key == temp->value.first)
            return temp;
    }
    return nullptr;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
typename HashMap<KEY,T,thash>::LN* HashMap<KEY,T,thash>::copy_list (LN* l) const {
    if(l->next == nullptr)
        return new LN();
    return new LN(l->value,copy_list(l->next));
}


template<class KEY,class T, int (*thash)(const KEY& a)>
typename HashMap<KEY,T,thash>::LN** HashMap<KEY,T,thash>::copy_hash_table (LN** ht, int bins) const {
    if(ht == nullptr)
        return nullptr;
    LN** newMap = new LN*[bins];
    for(int i = 0; i < bins; i++)
        newMap[i] = copy_list(ht[i]);
    return newMap;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::ensure_load_threshold(int new_used) {
    if(map == nullptr){
        map = new HashMap::LN*[bins];
        for(int j = 0; j < bins; j++)
            map[j] = new HashMap::LN();
    }
    if(((double) new_used / bins) <= load_threshold)
        return;
    int tempBins = bins;
    bins = bins * 2;
    HashMap::LN** tempMap = new HashMap::LN*[bins];
    for(int j = 0; j < bins; j++)
        tempMap[j] = new HashMap::LN();
    for(int i = 0; i < tempBins; i++){
        while(map[i]->next != nullptr){
            LN* temp2 = map[i];
            map[i] = map[i]->next;
            LN*& bucket = tempMap[hash_compress(temp2->value.first)];
            temp2->next = bucket;
            bucket = temp2;
        }
    }
    delete_hash_table(map,tempBins);
    map = tempMap;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::delete_hash_table (LN**& ht, int bins) {
    if(ht == nullptr)
        return;
    for(int i = 0; i < bins; i++){
        while(ht[i] != nullptr){
            LN* temp = ht[i];
            ht[i] = ht[i]->next;
            delete temp;
        }
    }
    delete[] ht;
    ht = nullptr;
}






////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class KEY,class T, int (*thash)(const KEY& a)>
void HashMap<KEY,T,thash>::Iterator::advance_cursors(){
    while(current.second->next == nullptr && current.first < ref_map->bins) {
        if(current.first + 1 >= ref_map->bins){
            current.first = -1;
            current.second = nullptr;
            break;
        }
        current.second = ref_map->map[++current.first];
    }
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::Iterator::Iterator(HashMap<KEY,T,thash>* iterate_over, bool from_begin)
: ref_map(iterate_over), expected_mod_count(ref_map->mod_count) {
    if(from_begin && (!(ref_map->empty()))){
        current = Cursor(0,ref_map->map[0]);
        advance_cursors();
    } else
        current = Cursor(-1,nullptr);
}


template<class KEY,class T, int (*thash)(const KEY& a)>
HashMap<KEY,T,thash>::Iterator::~Iterator()
{}


template<class KEY,class T, int (*thash)(const KEY& a)>
auto HashMap<KEY,T,thash>::Iterator::erase() -> Entry {
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("HashMap::Iterator::erase");
    if(!can_erase)
        throw ics::CannotEraseError("HashMap::Iterator::erase: Iterator cursor already erased");
    if(current.first == -1)
        throw ics::CannotEraseError("HashMap::Iterator::erase: Iterator cursor already beyond data structure");
    can_erase = false;
    Entry returnVal = current.second->value;
    HashMap::LN* to_del = current.second->next;
    *(current.second) = *(current.second->next);
    delete to_del;
    ref_map->used--;
    ref_map->mod_count++;
    expected_mod_count = ref_map->mod_count;
    advance_cursors();
    return returnVal;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
std::string HashMap<KEY,T,thash>::Iterator::str() const {
    std::ostringstream answer;
    answer << ref_map->str() << "(current_bin=" << current.first << ",current_node=" << current.second << ",expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
    return answer.str();
}

template<class KEY,class T, int (*thash)(const KEY& a)>
auto  HashMap<KEY,T,thash>::Iterator::operator ++ () -> HashMap<KEY,T,thash>::Iterator& {
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("HashMap::Iterator::operator ++");
    if(current.first == -1)
        return *this;
    if(can_erase){
        current.second = current.second->next;
        advance_cursors();
    } else
        can_erase = true;
    return *this;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
auto  HashMap<KEY,T,thash>::Iterator::operator ++ (int) -> HashMap<KEY,T,thash>::Iterator {
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("HashMap::Iterator::operator ++(int)");
    if(current.first == -1)
        return *this;
    Iterator to_return(*this);
    if(can_erase){
        current.second = current.second->next;
        advance_cursors();
    } else
        can_erase = true;
    return to_return;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::Iterator::operator == (const HashMap<KEY,T,thash>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if(rhsASI == 0)
        throw ics::IteratorTypeError("HashMap::Iterator::operator ==");
    if(ref_map->mod_count != expected_mod_count)
        throw ics::ConcurrentModificationError("HashMap::Iterator::operator ==");
    if(ref_map != rhsASI->ref_map)
        throw ics::ComparingDifferentIteratorsError("HashMap::Iterator::operator ==");
    return this->current == rhsASI->current;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
bool HashMap<KEY,T,thash>::Iterator::operator != (const HashMap<KEY,T,thash>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if(rhsASI == 0)
        throw ics::IteratorTypeError("HashMap::Iterator::operator !=");
    if(ref_map->mod_count != expected_mod_count)
        throw ics::ConcurrentModificationError("HashMap::Iterator::operator !=");
    if(ref_map != rhsASI->ref_map)
        throw ics::ComparingDifferentIteratorsError("HashMap::Iterator::operator !=");
    return this->current != rhsASI->current;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
pair<KEY,T>& HashMap<KEY,T,thash>::Iterator::operator *() const {
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("HashMap::Iterator::operator *");
    if(!can_erase || current.first == -1)
        throw ics::IteratorPositionIllegal("HashMap::Iterator::operator * Iterator illegal");
    return current.second->value;
}


template<class KEY,class T, int (*thash)(const KEY& a)>
pair<KEY,T>* HashMap<KEY,T,thash>::Iterator::operator ->() const {
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("HashMap::Iterator::operator ->");
    if(!can_erase || current.first == -1)
        throw ics::IteratorPositionIllegal("HashMap::Iterator::operator -> Iterator illegal");
    return &(current.second->value);
}


}

#endif /* HASH_MAP_HPP_ */
