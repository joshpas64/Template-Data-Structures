#ifndef BST_MAP_HPP_
#define BST_MAP_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"
#include "pair.hpp"
#include "array_queue.hpp"   //For traversal
// Submitter jpascasc(Pascascio, Joshua)

namespace ics {


#ifndef undefinedltdefined
#define undefinedltdefined
template<class T>
bool undefinedlt (const T& a, const T& b) {return false;}
#endif /* undefinedltdefined */

//Instantiate the templated class supplying tgt(a,b): true, iff a has higher priority than b.
//If tgt is defaulted to undefinedlt in the template, then a constructor must supply cgt.
//If both tlt and clt are supplied, then they must be the same (by ==) function.
//If neither is supplied, or both are supplied but different, TemplateFunctionError is raised.
//The (unique) non-undefinedlt value supplied by tlt/clt is stored in the instance variable gt.
template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b) = undefinedlt<KEY>> class BSTMap {
  public:
    typedef pair<KEY,T> Entry;
    typedef bool (*ltfunc) (const KEY& a, const KEY& b);

    //Destructor/Constructors
    ~BSTMap();

    BSTMap          (bool (*clt)(const KEY& a, const KEY& b) = undefinedlt<KEY>);
    BSTMap          (const BSTMap<KEY,T,tlt>& to_copy, bool (*clt)(const KEY& a, const KEY& b) = undefinedlt<KEY>);
    explicit BSTMap (const std::initializer_list<Entry>& il, bool (*clt)(const KEY& a, const KEY& b) = undefinedlt<KEY>);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit BSTMap (const Iterable& i, bool (*clt)(const KEY& a, const KEY& b) = undefinedlt<KEY>);


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
    BSTMap<KEY,T,tlt>& operator = (const BSTMap<KEY,T,tlt>& rhs);
    bool operator == (const BSTMap<KEY,T,tlt>& rhs) const;
    bool operator != (const BSTMap<KEY,T,tlt>& rhs) const;

    template<class KEY2,class T2, bool (*lt2)(const KEY2& a, const KEY2& b)>
    friend std::ostream& operator << (std::ostream& outs, const BSTMap<KEY2,T2,lt2>& m);



    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of BSTMap<T>
        ~Iterator();
        Entry       erase();
        std::string str  () const;
        BSTMap<KEY,T,tlt>::Iterator& operator ++ ();
        BSTMap<KEY,T,tlt>::Iterator  operator ++ (int);
        bool operator == (const BSTMap<KEY,T,tlt>::Iterator& rhs) const;
        bool operator != (const BSTMap<KEY,T,tlt>::Iterator& rhs) const;
        Entry& operator *  () const;
        Entry* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const BSTMap<KEY,T,tlt>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator BSTMap<KEY,T,tlt>::begin () const;
        friend Iterator BSTMap<KEY,T,tlt>::end   () const;

      private:
        //If can_erase is false, the value has been removed from "it" (++ does nothing)
        ArrayQueue<Entry> it;                 //Queue for all associations (from begin); use it as iterator with dequeue
        BSTMap<KEY,T,tlt>* ref_map;
        int               expected_mod_count;
        bool              can_erase = true;

        //Called in friends begin/end
        Iterator(BSTMap<KEY,T,tlt>* iterate_over, bool from_begin);
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    class TN {
      public:
        TN ()                     : left(nullptr), right(nullptr){}
        TN (const TN& tn)         : value(tn.value), left(tn.left), right(tn.right){}
        TN (Entry v, TN* l = nullptr,
                     TN* r = nullptr) : value(v), left(l), right(r){}

        Entry value;
        TN*   left;
        TN*   right;
    };

  bool (*lt) (const KEY& a, const KEY& b); // The lt used for searching BST (from template or constructor)
  TN* map       = nullptr;
  int used      = 0;                       //Cache the number of key->value pairs in the BST
  int mod_count = 0;                       //For sensing concurrent modification

  //Helper methods (find_key written iteratively, the rest recursively)
  TN*   find_key            (TN*  root, const KEY& key)                 const; //Returns reference to key's node or nullptr
  bool  has_value           (TN*  root, const T& value)                 const; //Returns whether value is is root's tree
  TN*   copy                (TN*  root)                                 const; //Copy the keys/values in root's tree (identical structure)
  void  copy_to_queue       (TN* root, ArrayQueue<Entry>& q)            const; //Fill queue with root's tree value
  bool  equals              (TN*  root, const BSTMap<KEY,T,tlt>& other) const; //Returns whether root's keys/value are all in other
  std::string string_rotated(TN* root, std::string indent)              const; //Returns string representing root's tree

  T     insert              (TN*& root, const KEY& key, const T& value);       //Put key->value, returning key's old value (or new one's, if key absent)
  T&    find_addempty       (TN*& root, const KEY& key);                       //Return reference to key's value (adding key->T() first, if key absent)
  Entry remove_closest      (TN*& root);                                       //Helper for remove
  T     remove              (TN*& root, const KEY& key);                       //Remove key->value from root's tree
  void  delete_BST          (TN*& root);                                       //Deallocate all TN in tree; root == nullptr
};





////////////////////////////////////////////////////////////////////////////////
//
//BSTMap class and related definitions

//Destructor/Constructors

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::~BSTMap() {
    delete_BST(map);
    delete map;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::BSTMap(bool (*clt)(const KEY& a, const KEY& b))
:lt(tlt != (ltfunc)undefinedlt<KEY> ? tlt : clt)
{
    if(lt == (ltfunc)undefinedlt<KEY>)
        throw ics::TemplateFunctionError("BSTMap::default constructor:neither specified");
    if(tlt != (ltfunc)undefinedlt<KEY> && clt != (ltfunc)undefinedlt<KEY> && tlt != clt)
        throw ics::TemplateFunctionError("BSTMap::default constructor:both specified and different");
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::BSTMap(const BSTMap<KEY,T,tlt>& to_copy, bool (*clt)(const KEY& a, const KEY& b))
:lt(tlt != (ltfunc)undefinedlt<KEY> ? tlt : clt)
{
    if(lt == (ltfunc)undefinedlt<KEY>)
        lt = to_copy.lt;
    if(tlt != (ltfunc)undefinedlt<KEY> && clt != (ltfunc)undefinedlt<KEY> && tlt != clt)
        throw ics::TemplateFunctionError("BSTMap::copy constructor: both specified and different");
    if(lt != to_copy.lt){
        for(BSTMap<KEY,T,tlt>::Iterator i = to_copy.begin(); i != to_copy.end(); ++i)
            put(i->first,i->second);
        mod_count = 0;
    }
    else{
        map = copy(to_copy.map);
        used = to_copy.used;
    }
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::BSTMap(const std::initializer_list<Entry>& il, bool (*clt)(const KEY& a, const KEY& b))
:lt(tlt != (ltfunc)undefinedlt<KEY> ? tlt : clt)
{
    if(lt == (ltfunc)undefinedlt<KEY>)
        throw ics::TemplateFunctionError("BSTMap::initializer_list constructor:neither specified");
    if(tlt != (ltfunc)undefinedlt<KEY> && clt != (ltfunc)undefinedlt<KEY> && tlt != clt)
        throw ics::TemplateFunctionError("BSTMap::initializer_list constructor: both specified and different");
    for(const Entry& e : il)
        put(e.first,e.second);
    mod_count = 0;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
template <class Iterable>
BSTMap<KEY,T,tlt>::BSTMap(const Iterable& i, bool (*clt)(const KEY& a, const KEY& b))
        :lt(tlt != (ltfunc)undefinedlt<KEY> ? tlt : clt)
{
    if(lt == (ltfunc)undefinedlt<KEY>)
        throw ics::TemplateFunctionError("BSTMap::Iterable constructor: neither specified");
    if(tlt != (ltfunc)undefinedlt<KEY> && clt != (ltfunc)undefinedlt<KEY> && tlt != clt)
        throw ics::TemplateFunctionError("BSTMap::Iterable constructor: both specified and different");
    for(const Entry& m : i)
        put(m.first,m.second);
    mod_count = 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::empty() const {
    return used == 0;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
int BSTMap<KEY,T,tlt>::size() const {
    return used;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::has_key (const KEY& key) const {
    return find_key(map,key) != nullptr;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::has_value (const T& value) const {
    return has_value(map,value);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
std::string BSTMap<KEY,T,tlt>::str() const {
    std::ostringstream answer;
    answer << "[";
    if(map != nullptr)
        answer << string_rotated(map,"");
    answer << "](used=" << used << ",mod_count=" << mod_count << ")";
    return answer.str();
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T BSTMap<KEY,T,tlt>::put(const KEY& key, const T& value) {
    return insert(map,key,value);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T BSTMap<KEY,T,tlt>::erase(const KEY& key) {
    T result = remove(map,key);
    --used;
    ++mod_count;
    return result;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
void BSTMap<KEY,T,tlt>::clear() {
    delete_BST(map);
    ++mod_count;
    used = 0;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
template<class Iterable>
int BSTMap<KEY,T,tlt>::put_all(const Iterable& i) {
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

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T& BSTMap<KEY,T,tlt>::operator [] (const KEY& key) {
    return find_addempty(map,key);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
const T& BSTMap<KEY,T,tlt>::operator [] (const KEY& key) const {
    TN* node = find_key(map,key);
    if(node == nullptr){
        std::ostringstream error;
        error << "BSTMap::operator[] (const T&) const: key(" << key << ") not in Map";
        throw ics::KeyError(error.str());
    }
    return node->value.second;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>& BSTMap<KEY,T,tlt>::operator = (const BSTMap<KEY,T,tlt>& rhs) {
    if(this == &rhs)
        return *this;
    delete_BST(map);
    if(lt != rhs.lt){
        for(BSTMap<KEY,T,tlt>::Iterator i = rhs.begin(); i != rhs.end(); ++i)
            put(i->first, i->second);
    }
    else{
        map = copy(rhs.map);
        used = rhs.used;
    }
    ++mod_count;
    return *this;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::operator == (const BSTMap<KEY,T,tlt>& rhs) const {
    if(this == &rhs)
        return true;
    if(used != rhs.used)
        return false;
    return equals(map,rhs);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::operator != (const BSTMap<KEY,T,tlt>& rhs) const {
    return !(*this == rhs);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
std::ostream& operator << (std::ostream& outs, const BSTMap<KEY,T,tlt>& m) {
    outs << "map[";
    if(!(m.empty())){
        int stop = m.size() - 1;
        int count = 0;
        for(const auto& e : m){
            outs << e.first << "->" << e.second;
            if(count < stop)
                outs << ",";
        }
    }
    outs << "]";
    return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
auto BSTMap<KEY,T,tlt>::begin () const -> BSTMap<KEY,T,tlt>::Iterator {
    return Iterator(const_cast<BSTMap<KEY,T,tlt>*>(this),true);
}

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
auto BSTMap<KEY,T,tlt>::end () const -> BSTMap<KEY,T,tlt>::Iterator {
    return Iterator(const_cast<BSTMap<KEY,T,tlt>*>(this),false);
}

////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
typename BSTMap<KEY,T,tlt>::TN* BSTMap<KEY,T,tlt>::find_key (TN* root, const KEY& key) const {
    TN* next;
    for(TN* temp = root; temp != nullptr; temp = next ){
        if(temp->value.first == key)
            return temp;
        if(lt(temp->value.first,key))
            next = temp->right;
        else
            next = temp->left;
    }
    return nullptr;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::has_value (TN* root, const T& value) const {
    if(root == nullptr)
        return false;
    if(root->value.second == value)
        return true;
    return (has_value(root->left,value) || has_value(root->right,value));
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
typename BSTMap<KEY,T,tlt>::TN* BSTMap<KEY,T,tlt>::copy (TN* root) const {
    if(root == nullptr)
        return nullptr;
    else
        return new TN(root->value,copy(root->left),copy(root->right));
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
void BSTMap<KEY,T,tlt>::copy_to_queue (TN* root, ArrayQueue<Entry>& q) const {
    if(root == nullptr)
        return;
    copy_to_queue(root->left,q);
    q.enqueue(root->value);
    copy_to_queue(root->right,q);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::equals (TN* root, const BSTMap<KEY,T,tlt>& other) const {
    if(root == nullptr)
        return true;
    else{
        if(!other.has_key(root->value.first))
            return false;
        if(root->value.second != other[root->value.first])
            return false;
        return equals(root->left,other) && equals(root->right,other);
    }
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
std::string BSTMap<KEY,T,tlt>::string_rotated(TN* root, std::string indent) const {
    if(root == nullptr)
        return "";
    std::ostringstream result;
    std::string new_indent = indent + "..";
    if(root->left != nullptr)
        result << string_rotated(root->left,new_indent);
    result << indent << root->value.first << "->" << root->value.second << "\n";
    if(root->right != nullptr)
        result << string_rotated(root->right,new_indent);
    return result.str();
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T BSTMap<KEY,T,tlt>::insert (TN*& root, const KEY& key, const T& value) {
    if(root == nullptr){
        ++used;
        ++mod_count;
        root = new TN(Entry(key,value));
        return root->value.second;
    }
    else{
        if(root->value.first == key){
            ++mod_count;
            T temp = root->value.second;
            root->value.second = value;
            return temp;
        }
        else{
            if(lt(root->value.first,key))
                return insert(root->right,key,value);
            else
                return insert(root->left,key,value);
        }
    }
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T& BSTMap<KEY,T,tlt>::find_addempty (TN*& root, const KEY& key) {
    if(root == nullptr){
        ++used;
        ++mod_count;
        root = new TN(Entry(key,T()));
        return root->value.second;
    }
    if(root->value.first == key) {
        return root->value.second;
    }
    else{
        if(lt(root->value.first,key))
            return find_addempty(root->right,key);
        else
            return find_addempty(root->left,key);
    }
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
pair<KEY,T> BSTMap<KEY,T,tlt>::remove_closest(TN*& root) {
  if (root->right != nullptr)
    return remove_closest(root->right);
  else{
    Entry to_return = root->value;
    TN* to_delete = root;
    root = root->left;
    delete to_delete;
    return to_return;
  }
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
T BSTMap<KEY,T,tlt>::remove (TN*& root, const KEY& key) {
  if (root == nullptr) {
    std::ostringstream answer;
    answer << "BSTMap::erase: key(" << key << ") not in Map";
    throw KeyError(answer.str());
  }else
    if (key == root->value.first) {
      T to_return = root->value.second;
      if (root->left == nullptr) {
        TN* to_delete = root;
        root = root->right;
        delete to_delete;
      }else if (root->right == nullptr) {
        TN* to_delete = root;
        root = root->left;
        delete to_delete;
      }else
        root->value = remove_closest(root->left);
      return to_return;
    }else
      return remove( (lt(key,root->value.first) ? root->left : root->right), key);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
void BSTMap<KEY,T,tlt>::delete_BST (TN*& root) {
    if(root == nullptr)
        return;
    else{
        TN* temp = root;
        if(root->right != nullptr)
            delete_BST(root->right);
        delete root->right;
        if(root->left != nullptr)
            delete_BST(root->left);
        delete root->left;
        root = nullptr;
        delete temp;
    }
}






////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::Iterator::Iterator(BSTMap<KEY,T,tlt>* iterate_over, bool from_begin)
{
    ref_map = iterate_over;
    expected_mod_count = ref_map->mod_count;
    if(from_begin)
        ref_map->copy_to_queue(ref_map->map,it);
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
BSTMap<KEY,T,tlt>::Iterator::~Iterator()
{}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
auto BSTMap<KEY,T,tlt>::Iterator::erase() -> Entry {
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("BSTMap::Iterator::erase");
    if(!can_erase)
        throw ics::CannotEraseError("BSTMap::Iterator::erase: Iterator cursor has already been erased");
    if(it.size() == 0)
        throw ics::CannotEraseError("BSTMap::Iterator::erase: Iterator cursor already beyond data structure");
    can_erase = false;
    Entry returnVal = it.dequeue();
    ref_map->erase(returnVal.first);
    expected_mod_count = ref_map->mod_count;
    return returnVal;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
std::string BSTMap<KEY,T,tlt>::Iterator::str() const {
    std::ostringstream result;
    result << ref_map->str() << "(it=" << it.str() << ",expected_mod_count="
           << expected_mod_count << ",can_erase=" << can_erase << ")";
    return result.str();
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
auto  BSTMap<KEY,T,tlt>::Iterator::operator ++ () -> BSTMap<KEY,T,tlt>::Iterator& {
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("BSTMap::Iterator::operator ++");
    if(it.size() == 0)
        return *this;
    if(can_erase)
        it.dequeue();
    else
        can_erase = true;
    return *this;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
auto BSTMap<KEY,T,tlt>::Iterator::operator ++ (int) -> BSTMap<KEY,T,tlt>::Iterator {
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("BSTMap::Iterator::operator ++(int)");
    if(it.size() == 0)
        return *this;
    Iterator to_return(*this);
    if(can_erase)
        it.dequeue();
    else
        can_erase = true;
    return to_return;
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::Iterator::operator == (const BSTMap<KEY,T,tlt>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if(rhsASI == 0)
        throw ics::IteratorTypeError("BSTMap::Iterator::operator ==");
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("BSTMap::Iterator::operator ==");
    if(ref_map != rhsASI->ref_map)
        throw ics::ComparingDifferentIteratorsError("BSTMap::Iterator::operator ==");
    return it.size() == rhsASI->it.size();
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
bool BSTMap<KEY,T,tlt>::Iterator::operator != (const BSTMap<KEY,T,tlt>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if(rhsASI == 0)
        throw ics::IteratorTypeError("BSTMap::Iterator::operator !=");
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("BSTMap::Iterator::operator !=");
    if(ref_map != rhsASI->ref_map)
        throw ics::ComparingDifferentIteratorsError("BSTMap::Iterator::operator !=");
    return it.size() != rhsASI->it.size();
}


template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
pair<KEY,T>& BSTMap<KEY,T,tlt>::Iterator::operator *() const {
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("BSTMap::Iterator::operator *");
    if(!can_erase || it.size() == 0)
        throw ics::IteratorPositionIllegal("BSTMap::Iterator::operator *:Iterator illegal");
    return it.peek();
}
template<class KEY,class T, bool (*tlt)(const KEY& a, const KEY& b)>
pair<KEY,T>* BSTMap<KEY,T,tlt>::Iterator::operator ->() const {
    if(expected_mod_count != ref_map->mod_count)
        throw ics::ConcurrentModificationError("BSTMap::Iterator::operator ->");
    if(!can_erase || it.size() == 0)
        throw ics::IteratorPositionIllegal("BSTMap::Iterator::operator ->:Iterator illegal");
    return &(it.peek());
}


}

#endif /* BST_MAP_HPP_ */
