#ifndef HASH_SET_HPP_
#define HASH_SET_HPP_

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
template<class T, int (*thash)(const T& a) = undefinedhash<T>> class HashSet {
  public:
    typedef int (*hashfunc) (const T& a);

    //Destructor/Constructors
    ~HashSet ();

    HashSet (double the_load_threshold = 1.0, int (*chash)(const T& a) = undefinedhash<T>);
    explicit HashSet (int initial_bins, double the_load_threshold = 1.0, int (*chash)(const T& k) = undefinedhash<T>);
    HashSet (const HashSet<T,thash>& to_copy, double the_load_threshold = 1.0, int (*chash)(const T& a) = undefinedhash<T>);
    explicit HashSet (const std::initializer_list<T>& il, double the_load_threshold = 1.0, int (*chash)(const T& a) = undefinedhash<T>);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit HashSet (const Iterable& i, double the_load_threshold = 1.0, int (*chash)(const T& a) = undefinedhash<T>);


    //Queries
    bool empty      () const;
    int  size       () const;
    bool contains   (const T& element) const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    bool contains_all (const Iterable& i) const;


    //Commands
    int  insert (const T& element);
    int  erase  (const T& element);
    void clear  ();

    //Iterable class must support "for" loop: .begin()/.end() and prefix ++ on returned result

    template <class Iterable>
    int insert_all(const Iterable& i);

    template <class Iterable>
    int erase_all(const Iterable& i);

    template<class Iterable>
    int retain_all(const Iterable& i);


    //Operators
    HashSet<T,thash>& operator = (const HashSet<T,thash>& rhs);
    bool operator == (const HashSet<T,thash>& rhs) const;
    bool operator != (const HashSet<T,thash>& rhs) const;
    bool operator <= (const HashSet<T,thash>& rhs) const;
    bool operator <  (const HashSet<T,thash>& rhs) const;
    bool operator >= (const HashSet<T,thash>& rhs) const;
    bool operator >  (const HashSet<T,thash>& rhs) const;

    template<class T2, int (*hash2)(const T2& a)>
    friend std::ostream& operator << (std::ostream& outs, const HashSet<T2,hash2>& s);



  private:
    class LN;

  public:
    class Iterator {
      public:
        typedef pair<int,LN*> Cursor;

        //Private constructor called in begin/end, which are friends of HashSet<T,thash>
        ~Iterator();
        T           erase();
        std::string str  () const;
        HashSet<T,thash>::Iterator& operator ++ ();
        HashSet<T,thash>::Iterator  operator ++ (int);
        bool operator == (const HashSet<T,thash>::Iterator& rhs) const;
        bool operator != (const HashSet<T,thash>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const HashSet<T,thash>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator HashSet<T,thash>::begin () const;
        friend Iterator HashSet<T,thash>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        Cursor              current; //Bin Index + LN* pointer; stops if LN* == nullptr
        HashSet<T,thash>*   ref_set;
        int                 expected_mod_count;
        bool                can_erase = true;

        //Helper methods
        void advance_cursors();

        //Called in friends begin/end
        Iterator(HashSet<T,thash>* iterate_over, bool from_begin);
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    class LN {
      public:
        LN ()                      {}
        LN (const LN& ln)          : value(ln.value), next(ln.next){}
        LN (T v,  LN* n = nullptr) : value(v), next(n){}

        T   value;
        LN* next   = nullptr;
    };

public:
  int (*hash)(const T& k);   //Hashing function used (from template or constructor)
private:
  LN** set      = nullptr;   //Pointer to array of pointers: each bin stores a list with a trailer node
  double load_threshold;     //used/bins <= load_threshold
  int bins      = 1;         //# bins in array (should start >= 1 so hash_compress doesn't divide by 0)
  int used      = 0;         //Cache for number of key->value pairs in the hash table
  int mod_count = 0;         //For sensing concurrent modification


  //Helper methods
  int   hash_compress        (const T& key)              const;  //hash function ranged to [0,bins-1]
  LN*   find_element         (const T& element)          const;  //Returns reference to element's node or nullptr
  LN*   copy_list            (LN*   l)                   const;  //Copy the elements in a bin (order irrelevant)
  LN**  copy_hash_table      (LN** ht, int bins)         const;  //Copy the bins/keys/values in ht tree (order in bins irrelevant)

  void  ensure_load_threshold(int new_used);                     //Reallocate if load_threshold > load_threshold
  void  delete_hash_table    (LN**& ht, int bins);               //Deallocate all LN in ht (and the ht itself; ht == nullptr)
};





//HashSet class and related definitions

////////////////////////////////////////////////////////////////////////////////
//
//Destructor/Constructors

template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::~HashSet() {
    delete_hash_table(set,bins);
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(double the_load_threshold, int (*chash)(const T& element))
:hash(thash != (hashfunc)undefinedhash<T> ? thash : chash),load_threshold(the_load_threshold)
{
    if(hash == (hashfunc)undefinedhash<T>)
        throw ics::TemplateFunctionError("HashSet::default constructor: neither specified");
    if(thash != (hashfunc)undefinedhash<T> && chash != (hashfunc)undefinedhash<T> && thash != chash)
        throw ics::TemplateFunctionError("HashSet::default constuctor: both specified and different");
    if(load_threshold <= 0)
        load_threshold = 1.0;
    ensure_load_threshold(used);
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(int initial_bins, double the_load_threshold, int (*chash)(const T& element))
:hash(thash != (hashfunc)undefinedhash<T> ? thash : chash),load_threshold(the_load_threshold),bins(initial_bins)
{
    if(hash == (hashfunc)undefinedhash<T>)
        throw ics::TemplateFunctionError("HashSet::initial_bins constructor: neither specified");
    if(thash != (hashfunc)undefinedhash<T> && chash != (hashfunc)undefinedhash<T> && thash != chash)
        throw ics::TemplateFunctionError("HashSet::initial_bins constructor: both specified and different");
    if(load_threshold <= 0)
        load_threshold = 1.0;
    if(bins < 1)
        bins = 1;
    ensure_load_threshold(used);
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(const HashSet<T,thash>& to_copy, double the_load_threshold, int (*chash)(const T& element))
:hash(thash != (hashfunc)undefinedhash<T> ? thash : chash), load_threshold(the_load_threshold)
{
    if(hash == (hashfunc)undefinedhash<T>)
        hash = to_copy.hash;
    if(thash != (hashfunc)undefinedhash<T> && chash != (hashfunc)undefinedhash<T> && thash != chash)
        throw ics::TemplateFunctionError("HashSet::copy constructor: both specified and different");
    if(load_threshold <= 0)
        load_threshold = 1.0;
    if(hash == to_copy.hash){
        used = to_copy.used;
        bins = to_copy.bins;
        set = copy_hash_table(to_copy.set,bins);
    }
    else {
        for (const T &elem : to_copy)
            insert(elem);
    }

}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::HashSet(const std::initializer_list<T>& il, double the_load_threshold, int (*chash)(const T& element))
:hash(thash != (hashfunc)undefinedhash<T> ? thash : chash),load_threshold(the_load_threshold)
{
    if(hash == (hashfunc)undefinedhash<T>)
        throw ics::TemplateFunctionError("HashSet::initializer_list constructor: neither specified");
    if(thash != (hashfunc)undefinedhash<T> && chash != (hashfunc)undefinedhash<T> && thash != chash)
        throw ics::TemplateFunctionError("HashSet::initializer_list constructor: both specified and different");
    if(load_threshold <= 0)
        load_threshold = 1.0;
    for(const T& i : il)
        insert(i);
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
HashSet<T,thash>::HashSet(const Iterable& i, double the_load_threshold, int (*chash)(const T& a))
:hash(thash != (hashfunc)undefinedhash<T> ? thash : chash),load_threshold(the_load_threshold)
{
    if(hash == (hashfunc)undefinedhash<T>)
        throw ics::TemplateFunctionError("HashSet::Iterable constructor: neither specified");
    if(thash != (hashfunc)undefinedhash<T> && chash != (hashfunc)undefinedhash<T> && thash != chash)
        throw ics::TemplateFunctionError("HashSet::Iterable constructor: both specified and different");
    if(load_threshold <= 0)
        load_threshold = 1.0;
    for(const T& elem : i)
        insert(elem);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::empty() const {
    return used == 0;
}


template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::size() const {
    return used;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::contains (const T& element) const {
    return find_element(element) != nullptr;
}


template<class T, int (*thash)(const T& a)>
std::string HashSet<T,thash>::str() const {
    std::ostringstream answer;
    answer << "HashSet[";
    if(!(this->empty())){
        for(int i = 0; i < bins; i++){
            answer << "\nbins[" << i << "]:  ";
            for(HashSet::LN* temp = set[i]; temp->next != nullptr; temp = temp->next)
                answer << temp->value << "->";
            answer << "TRAILER";
        }
    }
    answer << "](used=" << used << ",bins=" << bins << ",mod_count=" << mod_count << ")";
    return answer.str();
}


template<class T, int (*thash)(const T& a)>
template <class Iterable>
bool HashSet<T,thash>::contains_all(const Iterable& i) const {
    for(auto v : i){
        if(!contains(v))
            return false;
    }
    return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::insert(const T& element) {
    LN* temp = find_element(element);
    if(temp != nullptr)
        return 0;
    ++mod_count;
    ensure_load_threshold(++used);
    int index = hash_compress(element);
    set[index] = new LN(element,set[index]);
    return 1;
}


template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::erase(const T& element) {
    LN* temp = find_element(element);
    if(temp == nullptr)
        return 0;
    LN* to_delete = temp->next;
    *temp = *(temp->next);
    delete to_delete;
    --used;
    ++mod_count;
    return 1;
}


template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::clear() {
    delete_hash_table(set,bins);
    used = 0;
    ++mod_count;
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
int HashSet<T,thash>::insert_all(const Iterable& i) {
    int count = 0;
    for(const T& elem : i)
        count += insert(elem);
    return count;
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
int HashSet<T,thash>::erase_all(const Iterable& i) {
    int count = 0;
    for(const T& elem : i)
        count += erase(elem);
    return count;
}


template<class T, int (*thash)(const T& a)>
template<class Iterable>
int HashSet<T,thash>::retain_all(const Iterable& i) {
    HashSet hset(i);
    int count = 0;
    for(HashSet::Iterator i = this->begin(); i != this->end(); ++i){
        if(!hset.contains(*i)){
            i.erase();
            ++count;
        }
    }
    return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T, int (*thash)(const T& a)>
HashSet<T,thash>& HashSet<T,thash>::operator = (const HashSet<T,thash>& rhs) {
    if(this == &rhs)
        return *this;
    clear();
    if(hash == rhs.hash){
        set = copy_hash_table(rhs.set,rhs.bins);
        used = rhs.used;
        bins = rhs.bins;
    }
    else{
        hash = rhs.hash;
        insert_all(rhs);
    }
    ++mod_count;
    return *this;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator == (const HashSet<T,thash>& rhs) const {
    if(this == &rhs)
        return true;
    if(used != rhs.used)
        return false;
    HashSet::LN* temp;
    for(HashSet::Iterator i = rhs.begin(); i != rhs.end(); ++i){
        temp = find_element(*i);
        if(temp == nullptr)
            return false;
    }
    return true;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator != (const HashSet<T,thash>& rhs) const {
    return !(*this == rhs);
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator <= (const HashSet<T,thash>& rhs) const {
    if(this == &rhs)
        return true;
    if(used > rhs.used)
        return false;
    for(HashSet::Iterator i = this->begin(); i != this->end(); ++i){
        if(rhs.find_element(*i) == nullptr)
            return false;
    }
    return true;
}

template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator < (const HashSet<T,thash>& rhs) const {
    if(this == &rhs)
        return false;
    if(used >= rhs.used)
        return false;
    for(HashSet::Iterator i = this->begin(); i != this->end(); ++i){
        if(rhs.find_element(*i) == nullptr)
            return false;
    }
    return true;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator >= (const HashSet<T,thash>& rhs) const {
    return rhs <= *this;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::operator > (const HashSet<T,thash>& rhs) const {
    return rhs < *this;
}


template<class T, int (*thash)(const T& a)>
std::ostream& operator << (std::ostream& outs, const HashSet<T,thash>& s) {
    outs << "set[";
    if(!s.empty()){
        typename HashSet<T,thash>::Iterator i = s.begin();
        outs << *i;
        ++i;
        for(; i != s.end(); ++i)
            outs << "," << *i;
    }
    outs << "]";
    return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class T, int (*thash)(const T& a)>
auto HashSet<T,thash>::begin () const -> HashSet<T,thash>::Iterator {
    return Iterator(const_cast<HashSet<T,thash>*>(this),true);
}


template<class T, int (*thash)(const T& a)>
auto HashSet<T,thash>::end () const -> HashSet<T,thash>::Iterator {
    return Iterator(const_cast<HashSet<T,thash>*>(this),false);
}


///////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T, int (*thash)(const T& a)>
int HashSet<T,thash>::hash_compress (const T& element) const {
    int result = hash(element) % bins;
    if(result < 0)
        result = result * -1;
    return result;
}


template<class T, int (*thash)(const T& a)>
typename HashSet<T,thash>::LN* HashSet<T,thash>::find_element (const T& element) const {
    if(set == nullptr)
        return nullptr;
    for(LN* temp = set[hash_compress(element)]; temp->next != nullptr; temp = temp->next){
        if(temp->value == element)
            return temp;
    }
    return nullptr;
}

template<class T, int (*thash)(const T& a)>
typename HashSet<T,thash>::LN* HashSet<T,thash>::copy_list (LN* l) const {
    if(l->next == nullptr)
        return new LN();
    return new LN(l->value,copy_list(l->next));
}


template<class T, int (*thash)(const T& a)>
typename HashSet<T,thash>::LN** HashSet<T,thash>::copy_hash_table (LN** ht, int bins) const {
    if(ht == nullptr)
        return nullptr;
    LN** newSet = new LN*[bins];
    for(int i = 0; i < bins; i++)
        newSet[i] = copy_list(ht[i]);
    return newSet;
}


template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::ensure_load_threshold(int new_used) {
    if(set == nullptr){
        set = new HashSet::LN*[bins];
        for(int i = 0; i < bins; i++)
            set[i] = new HashSet::LN();
    }
    if(((double) new_used / bins) <= load_threshold)
        return;
    int tempBins = bins;
    bins = bins * 2;
    HashSet::LN** newSet = new HashSet::LN*[bins];
    for(int j = 0; j < bins; j++)
        newSet[j] = new HashSet::LN();
    for(int k = 0; k < tempBins; k++){
        while(set[k]->next != nullptr){
            LN* temp2 = set[k];
            set[k] = set[k]->next;
            LN*& newBin = newSet[hash_compress(temp2->value)];
            temp2->next = newBin;
            newBin = temp2;
        }
    }
    delete_hash_table(set,tempBins);
    set = newSet;
}


template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::delete_hash_table (LN**& ht, int bins) {
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

template<class T, int (*thash)(const T& a)>
void HashSet<T,thash>::Iterator::advance_cursors() {
    while(current.second->next == nullptr && current.first < ref_set->bins){
        if(current.first + 1 >= ref_set->bins){
            current.first = -1;
            current.second = nullptr;
            break;
        }
        current.second = ref_set->set[++current.first];
    }
}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::Iterator::Iterator(HashSet<T,thash>* iterate_over, bool begin)
:ref_set(iterate_over),expected_mod_count(ref_set->mod_count)
{
    if(begin && !ref_set->empty()){
        current = Cursor(0,ref_set->set[0]);
        advance_cursors();
    } else
        current = Cursor(-1,nullptr);

}


template<class T, int (*thash)(const T& a)>
HashSet<T,thash>::Iterator::~Iterator()
{}


template<class T, int (*thash)(const T& a)>
T HashSet<T,thash>::Iterator::erase() {
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("HashSet::Iterator::erase");
    if(!can_erase)
        throw ics::CannotEraseError("HashSet::Iterator::erase: Iterator cursor already erased");
    if(current.first == -1)
        throw ics::CannotEraseError("HashSet::Iterator::erase: Iterator cursor already beyond data structure");
    can_erase = false;
    T returnVal = current.second->value;
    HashSet::LN* to_del = current.second->next;
    *(current.second) = *(current.second->next);
    delete to_del;
    ref_set->used--;
    ref_set->mod_count++;
    expected_mod_count = ref_set->mod_count;
    advance_cursors();
    return returnVal;
}


template<class T, int (*thash)(const T& a)>
std::string HashSet<T,thash>::Iterator::str() const {
    std::ostringstream answer;
    answer << ref_set->str() << "(current_bin=" << current.first << ",current_node=" << current.second << ",expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
    return answer.str();
}


template<class T, int (*thash)(const T& a)>
auto  HashSet<T,thash>::Iterator::operator ++ () -> HashSet<T,thash>::Iterator& {
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("HashSet::Iterator:operator ++");
    if(current.first == -1)
        return *this;
    if(can_erase){
        current.second = current.second->next;
        advance_cursors();
    } else
        can_erase = true;
    return *this;
}


template<class T, int (*thash)(const T& a)>
auto  HashSet<T,thash>::Iterator::operator ++ (int) -> HashSet<T,thash>::Iterator {
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("HashSet::Iterator:operator ++(int)");
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


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::Iterator::operator == (const HashSet<T,thash>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if(rhsASI == 0)
        throw ics::IteratorTypeError("HashSet::Iterator::operator ==");
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("HashSet::Iterator::operator ==");
    if(ref_set != rhsASI->ref_set)
        throw ics::ComparingDifferentIteratorsError("HashSet::Iterator::operator ==");
    return this->current == rhsASI->current;
}


template<class T, int (*thash)(const T& a)>
bool HashSet<T,thash>::Iterator::operator != (const HashSet<T,thash>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if(rhsASI == 0)
        throw ics::IteratorTypeError("HashSet::Iterator::operator !=");
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("HashSet::Iterator::operator !=");
    if(ref_set != rhsASI->ref_set)
        throw ics::ComparingDifferentIteratorsError("HashSet::Iterator::operator !=");
    return this->current != rhsASI->current;
}

template<class T, int (*thash)(const T& a)>
T& HashSet<T,thash>::Iterator::operator *() const {
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("HashSet::Iterator::operator *");
    if(!can_erase || current.first == -1)
        throw ics::IteratorPositionIllegal("HashSet::Iterator::operator *: Iterator illegal");
    return current.second->value;
}

template<class T, int (*thash)(const T& a)>
T* HashSet<T,thash>::Iterator::operator ->() const {
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("HashSet::Iterator::operator ->");
    if(!can_erase || current.first == -1)
        throw ics::IteratorPositionIllegal("HashSet::Iterator::operator ->: Iterator illegal");
    return &(current.second->value);
}

}

#endif /* HASH_SET_HPP_ */
