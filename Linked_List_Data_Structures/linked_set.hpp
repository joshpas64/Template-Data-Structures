// Submitter : jpascasc(Pascascio, Joshua)

#ifndef LINKED_SET_HPP_
#define LINKED_SET_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"

namespace ics {
template<class T> class LinkedSet {
  public:
    //Destructor/Constructors
    ~LinkedSet();
    LinkedSet          ();
    explicit LinkedSet (int initialLength);
    LinkedSet          (const LinkedSet<T>& to_copy);
    explicit LinkedSet (const std::initializer_list<T>& il);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit LinkedSet (const Iterable& i);


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
    LinkedSet<T>& operator = (const LinkedSet<T>& rhs);
    bool operator == (const LinkedSet<T>& rhs) const;
    bool operator != (const LinkedSet<T>& rhs) const;
    bool operator <= (const LinkedSet<T>& rhs) const;
    bool operator <  (const LinkedSet<T>& rhs) const;
    bool operator >= (const LinkedSet<T>& rhs) const;
    bool operator >  (const LinkedSet<T>& rhs) const;

    template<class T2>
    friend std::ostream& operator << (std::ostream& outs, const LinkedSet<T2>& s);



  private:
    class LN;

  public:
    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of LinkedSet<T>
        ~Iterator();
        T           erase();
        std::string str  () const;
        LinkedSet<T>::Iterator& operator ++ ();
        LinkedSet<T>::Iterator  operator ++ (int);
        bool operator == (const LinkedSet<T>::Iterator& rhs) const;
        bool operator != (const LinkedSet<T>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const LinkedSet<T>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator LinkedSet<T>::begin () const;
        friend Iterator LinkedSet<T>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        LN*           current;  //if can_erase is false, this value is unusable
        LinkedSet<T>* ref_set;
        int           expected_mod_count;
        bool          can_erase = true;

        //Called in friends begin/end
        Iterator(LinkedSet<T>* iterate_over, LN* initial);
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


    LN* front     = new LN();
    LN* trailer   = front;         //Always point to the special trailer LN
    int used      =  0;            //Cache the number of values in linked list
    int mod_count = 0;             //For sensing concurrent modification

    //Helper methods
    int  erase_at   (LN* p);
    void delete_list(LN*& front);  //Deallocate all LNs (but trailer), and set front's argument to trailer;
};





////////////////////////////////////////////////////////////////////////////////
//
//LinkedSet class and related definitions

//Destructor/Constructors

template<class T>
LinkedSet<T>::~LinkedSet() {
    clear();
    delete trailer;
}


template<class T>
LinkedSet<T>::LinkedSet() {
}


template<class T>
LinkedSet<T>::LinkedSet(const LinkedSet<T>& to_copy) : used(to_copy.used) {
    LinkedSet<T>::LN* temp = front;
    trailer = front;
    for(LinkedSet<T>::LN* start = to_copy.front; start != to_copy.trailer; start = start->next){
        LinkedSet<T>::LN* copy = front;
        front = new LinkedSet<T>::LN(start->value,copy);
    }
}


template<class T>
LinkedSet<T>::LinkedSet(const std::initializer_list<T>& il) {
    for(const T& i : il)
        insert(i);
}


template<class T>
template<class Iterable>
LinkedSet<T>::LinkedSet(const Iterable& i) {
    for(const T& e : i)
        insert(e);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T>
bool LinkedSet<T>::empty() const {
    return used == 0;
}


template<class T>
int LinkedSet<T>::size() const {
    return used;
}


template<class T>
bool LinkedSet<T>::contains (const T& element) const {
    for(LinkedSet<T>::LN* temp = front; temp != trailer; temp = temp->next){
        if(temp->value == element)
            return true;
    }
    return false;
}


template<class T>
std::string LinkedSet<T>::str() const {
    std::ostringstream answer;
    answer << "linked_set[";
    if(!(this->empty())){
        for(LinkedSet<T>::LN* temp = front; temp != trailer; temp = temp->next)
            answer << temp->value << "->";
    }
    answer << "TRAILER](used=" << used << ",front=" << front << ",trailer=" << trailer << ",mod_count=" << mod_count << ")";
    return answer.str();
}


template<class T>
template<class Iterable>
bool LinkedSet<T>::contains_all (const Iterable& i) const {
    for(const T& t : i){
        if(!(contains(t)))
            return false;
    }
    return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands


template<class T>
int LinkedSet<T>::insert(const T& element) {
    for(LinkedSet<T>::LN* temp = front; temp != trailer; temp = temp->next){
        if(temp->value == element)
            return 0;
    }
    if(front == trailer)
        front = new LinkedSet<T>::LN(element,trailer);
    else{
        LinkedSet<T>::LN* copy = front;
        front = new LinkedSet<T>::LN(element,copy);
    }
    ++used;
    ++mod_count;
    return 1;
}


template<class T>
int LinkedSet<T>::erase(const T& element) {
    for(LinkedSet<T>::LN* temp = front; temp != trailer; temp = temp->next){
        if(temp->value == element) {
            erase_at(temp);
            if(temp->next == nullptr)
                trailer = temp;
            return 1;
        }
    }
    return 0;
}


template<class T>
void LinkedSet<T>::clear() {
    delete_list(front);
}


template<class T>
template<class Iterable>
int LinkedSet<T>::insert_all(const Iterable& i) {
    int count = 0;
    for(const T& e: i)
        count += insert(e);
    return count;
}


template<class T>
template<class Iterable>
int LinkedSet<T>::erase_all(const Iterable& i) {
    int count = 0;
    for(const T& e : i)
        count += erase(e);
    return count;
}


template<class T>
template<class Iterable>
int LinkedSet<T>::retain_all(const Iterable& i) {
    int count = 0;
    LinkedSet<T> a;
    a.insert_all(i);
    LinkedSet<T>::LN* node = front;
    while(node != trailer){
        if(!a.contains(node->value)){
            count += erase_at(node);
            if(node->next == nullptr)
                trailer = node;
        } else
            node = node->next;
    }
    return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T>
LinkedSet<T>& LinkedSet<T>::operator = (const LinkedSet<T>& rhs) {
    if(this == &rhs)
        return *this;
    int difference = used - rhs.used;
    used = rhs.used;
    mod_count += rhs.used;
    for(int i = 0 ; i < difference; i++){
        LinkedSet<T>::LN* temp = front;
        front = front->next;
        delete temp;
    }
    LinkedSet<T>::LN* test = rhs.front;
    std::cout << *this << "  " << rhs << std::endl;
    for(LinkedSet<T>::LN** temp = &front; (*temp) != trailer; temp = &((*temp)->next)){
        if(test != rhs.trailer){
            (*temp)->value = test->value;
            test = test->next;
        }
    }
    for(; test != rhs.trailer; test = test->next){
        LinkedSet<T>::LN* temp = front;
        front = new LinkedSet<T>::LN(test->value, temp);
    }
    return *this;
}


template<class T>
bool LinkedSet<T>::operator == (const LinkedSet<T>& rhs) const {
    if(this == &rhs)
        return true;
    if(used != rhs.used)
        return false;
    for(LinkedSet<T>::LN* temp = front; temp != trailer; temp = temp->next){
        if(!(rhs.contains(temp->value)))
            return false;
    }
    return true;
}


template<class T>
bool LinkedSet<T>::operator != (const LinkedSet<T>& rhs) const {
    return !(*this == rhs);
}


template<class T>
bool LinkedSet<T>::operator <= (const LinkedSet<T>& rhs) const {
    if(this == &rhs)
        return true;
    if(used > rhs.used)
        return false;
    for(LinkedSet<T>::LN* temp = front; temp != trailer; temp = temp->next){
        if(!(rhs.contains(temp->value)))
            return false;
    }
    return true;
}


template<class T>
bool LinkedSet<T>::operator < (const LinkedSet<T>& rhs) const {
    if(this == &rhs)
        return false;
    if(used >= rhs.used)
        return false;
    for(LinkedSet<T>::LN* temp = front; temp != trailer; temp = temp->next){
        if(!(rhs.contains(temp->value)))
            return false;
    }
    return true;
}


template<class T>
bool LinkedSet<T>::operator >= (const LinkedSet<T>& rhs) const {
    return rhs <= *this;
}


template<class T>
bool LinkedSet<T>::operator > (const LinkedSet<T>& rhs) const {
    return rhs < *this;
}


template<class T>
std::ostream& operator << (std::ostream& outs, const LinkedSet<T>& s) {
    outs << "set[";
    int index = 0;
    if(!(s.empty())){
        for(auto &node : s){
            outs << node;
            if(index < (s.size() - 1))
                outs << ",";
            index++;
        }
    }
    outs << "]";
    return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class T>
auto LinkedSet<T>::begin () const -> LinkedSet<T>::Iterator {
    return Iterator(const_cast<LinkedSet<T>*>(this),this->front);
}


template<class T>
auto LinkedSet<T>::end () const -> LinkedSet<T>::Iterator {
    return Iterator(const_cast<LinkedSet<T>*>(this),this->trailer);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T>
int LinkedSet<T>::erase_at(LN* p) {
    LinkedSet<T>::LN* temp = p->next;
    *p = *temp;
    delete temp;
    ++mod_count;
    --used;
    return 1;
}


template<class T>
void LinkedSet<T>::delete_list(LN*& front) {
    while(front != trailer){
        LinkedSet<T>::LN* temp = front;
        front = front->next;
        delete temp;
        ++mod_count;
        --used;
    }
   trailer = front;
}





////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T>
LinkedSet<T>::Iterator::Iterator(LinkedSet<T>* iterate_over, LN* initial)
{
    ref_set = iterate_over;
    current = initial;
    expected_mod_count = ref_set->mod_count;
}


template<class T>
LinkedSet<T>::Iterator::~Iterator()
{}


template<class T>
T LinkedSet<T>::Iterator::erase() {
    if(ref_set->mod_count != expected_mod_count)
        throw ics::ConcurrentModificationError("LinkedSet::Iterator::erase");
    if(!can_erase)
        throw ics::CannotEraseError("LinkedSet::Iterator::erase Iterator cursor already erased");
    if(current == ref_set->trailer)
        throw ics::IteratorPositionIllegal("LinkedSet::Iterator::erase Iterator cursor beyond data structure");
    T returnVal = current->value;
    can_erase = false;
    LinkedSet<T>::LN* temp = current;
    ref_set->erase(current->value);
    expected_mod_count = ref_set->mod_count;
    return returnVal;
}


template<class T>
std::string LinkedSet<T>::Iterator::str() const {
    std::ostringstream answer;
    answer << ref_set->str() << "(current=" << current << ",expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
    return answer.str();
}


template<class T>
auto LinkedSet<T>::Iterator::operator ++ () -> LinkedSet<T>::Iterator& {
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("LinkedSet::Iterator::operator ++");
    if(current == ref_set->trailer)
        return *this;
    if(can_erase)
        current = current->next;
    else
        can_erase = true;
    return *this;
}


template<class T>
auto LinkedSet<T>::Iterator::operator ++ (int) -> LinkedSet<T>::Iterator {
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("LinkedSet::Iterator::operator ++(int)");
    if(current == ref_set->trailer)
        return *this;
    Iterator to_return(*this);
    if(can_erase)
        current = current->next;
    else
        can_erase = true;
    return to_return;
}


template<class T>
bool LinkedSet<T>::Iterator::operator == (const LinkedSet<T>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if(rhsASI == 0)
        throw ics::IteratorTypeError("LinkedSet::Iterator::operator ==");
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("LinkedSet::Iterator::operator ==");
    if(ref_set != rhsASI->ref_set)
        throw ics::ComparingDifferentIteratorsError("LinkedSet::Iterator::operator ==");
    return current == rhsASI->current;
}


template<class T>
bool LinkedSet<T>::Iterator::operator != (const LinkedSet<T>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if(rhsASI == 0)
        throw ics::IteratorTypeError("LinkedSet::Iterator::operator !=");
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("LinkedSet::Iterator::operator !=");
    if(ref_set != rhsASI->ref_set)
        throw ics::ComparingDifferentIteratorsError("LinkedSet::Iterator::operator !=");
    return current != rhsASI->current;
}


template<class T>
T& LinkedSet<T>::Iterator::operator *() const {
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("LinkedSet::Iterator::operator *");
    if(!can_erase || current == ref_set->trailer)
        throw ics::IteratorPositionIllegal("LinkedSet::Iterator::operator *");
    return current->value;
}


template<class T>
T* LinkedSet<T>::Iterator::operator ->() const {
    if(expected_mod_count != ref_set->mod_count)
        throw ics::ConcurrentModificationError("LinkedSet::Iterator::operator ->");
    if(!can_erase || current == ref_set->trailer)
        throw ics::IteratorPositionIllegal("LinkedSet::Iterator::operator ->");
    return &current->value;
}


}

#endif /* LINKED_SET_HPP_ */
