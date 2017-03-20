// Submitter : jpascasc(Pascascio, Joshua)
#ifndef LINKED_PRIORITY_QUEUE_HPP_
#define LINKED_PRIORITY_QUEUE_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"
#include "array_stack.hpp"      //See operator <<


namespace ics {


#ifndef undefinedgtdefined
#define undefinedgtdefined
template<class T>
bool undefinedgt (const T& a, const T& b) {return false;}
#endif /* undefinedgtdefined */

//Instantiate the templated class supplying tgt(a,b): true, iff a has higher priority than b.
//If tgt is defaulted to undefinedgt in the template, then a constructor must supply cgt.
//If both tgt and cgt are supplied, then they must be the same (by ==) function.
//If neither is supplied, or both are supplied but different, TemplateFunctionError is raised.
//The (unique) non-undefinedgt value supplied by tgt/cgt is stored in the instance variable gt.
template<class T, bool (*tgt)(const T& a, const T& b) = undefinedgt<T>> class LinkedPriorityQueue {
  public:
        typedef bool (*gtfunc)(const T &a, const T &b);
    //Destructor/Constructors
    ~LinkedPriorityQueue();

    LinkedPriorityQueue          (bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);
    LinkedPriorityQueue          (const LinkedPriorityQueue<T,tgt>& to_copy, bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);
    explicit LinkedPriorityQueue (const std::initializer_list<T>& il, bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit LinkedPriorityQueue (const Iterable& i, bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);


    //Queries
    bool empty      () const;
    int  size       () const;
    T&   peek       () const;
    std::string str () const; //supplies useful debugging information; contrast to operator <<


    //Commands
    int  enqueue (const T& element);
    T    dequeue ();
    void clear   ();

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    int enqueue_all (const Iterable& i);


    //Operators
    LinkedPriorityQueue<T,tgt>& operator = (const LinkedPriorityQueue<T,tgt>& rhs);
    bool operator == (const LinkedPriorityQueue<T,tgt>& rhs) const;
    bool operator != (const LinkedPriorityQueue<T,tgt>& rhs) const;

    template<class T2, bool (*gt2)(const T2& a, const T2& b)>
    friend std::ostream& operator << (std::ostream& outs, const LinkedPriorityQueue<T2,gt2>& pq);



  private:
    class LN;

  public:
    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of LinkedPriorityQueue<T,tgt>
        ~Iterator();
        T           erase();
        std::string str  () const;
        LinkedPriorityQueue<T,tgt>::Iterator& operator ++ ();
        LinkedPriorityQueue<T,tgt>::Iterator  operator ++ (int);
        bool operator == (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const;
        bool operator != (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const LinkedPriorityQueue<T,tgt>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator LinkedPriorityQueue<T,tgt>::begin () const;
        friend Iterator LinkedPriorityQueue<T,tgt>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        LN*             prev;            //initialize prev to the header
        LN*             current;         //current == prev->next
        LinkedPriorityQueue<T,tgt>* ref_pq;
        int             expected_mod_count;
        bool            can_erase = true;

        //Called in friends begin/end
        Iterator(LinkedPriorityQueue<T,tgt>* iterate_over, LN* initial);
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
        LN* next = nullptr;
    };


    bool (*gt) (const T& a, const T& b); // The gt used by enqueue (from template or constructor)
    LN* front     =  new LN();
    int used      =  0;                  //Cache count of nodes in linked list
    int mod_count =  0;                  //Allows sensing concurrent modification

    //Helper methods
    void delete_list(LN*& front);        //Deallocate all LNs, and set front's argument to nullptr;
};





////////////////////////////////////////////////////////////////////////////////
//
//LinkedPriorityQueue class and related definitions

//Destructor/Constructors

template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::~LinkedPriorityQueue() {
    delete_list(front);
    delete front;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(bool (*cgt)(const T& a, const T& b))
:gt(tgt != (gtfunc)undefinedgt<T> ? tgt : cgt)
{
    if(gt == (gtfunc)undefinedgt<T>)
        throw ics::TemplateFunctionError("LinkedPriorityQueue::default constructor: neither specified");
    if(tgt != (gtfunc)undefinedgt<T> && cgt != (gtfunc)undefinedgt<T> && tgt != cgt)
        throw ics::TemplateFunctionError("LinkedPriorityQueue::default constructor: both specified and different");
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(const LinkedPriorityQueue<T,tgt>& to_copy, bool (*cgt)(const T& a, const T& b))
:gt(tgt != (gtfunc)undefinedgt<T> ? tgt : cgt)
{
    if(gt == (gtfunc)undefinedgt<T>)
        gt = to_copy.gt;
    if(tgt != (gtfunc)undefinedgt<T> && cgt != (gtfunc)undefinedgt<T> && tgt != cgt)
        throw ics::TemplateFunctionError("LinkedPriorityQueue::copy constructorL both specified and different");
    LinkedPriorityQueue<T,tgt>::LN* rear = front;
    if(gt == to_copy.gt){
        used = to_copy.used;
        for(LinkedPriorityQueue<T,tgt>::LN* temp = to_copy.front; temp->next != nullptr; temp=temp->next){
            rear->next = new LinkedPriorityQueue<T,tgt>::LN(temp->next->value);
            rear = rear->next;
        }
    } else{
        for(LinkedPriorityQueue<T,tgt>::LN* temp = to_copy.front; temp->next != nullptr; temp = temp->next) {
            enqueue(temp->next->value);
        }
    }
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(const std::initializer_list<T>& il, bool (*cgt)(const T& a, const T& b))
:gt(tgt != undefinedgt<T> ? tgt : cgt)
{
    if(gt == undefinedgt<T>)
        throw ics::TemplateFunctionError("LinkedPriorityQueue::initializer_list_constructor: neither specified");
    if (tgt != undefinedgt<T> && cgt != undefinedgt<T> && tgt != cgt)
        throw ics::TemplateFunctionError("LinkedPriorityQueue::initializer_list constructor: both specified and different");
    for(const T& i : il)
        enqueue(i);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
template<class Iterable>
LinkedPriorityQueue<T,tgt>::LinkedPriorityQueue(const Iterable& i, bool (*cgt)(const T& a, const T& b))
        :gt(tgt != undefinedgt<T> ? tgt : cgt)
{
    if(gt == undefinedgt<T>)
        throw ics::TemplateFunctionError("LinkedPriorityQueue::Iterable constructor: neither specified");
    if (tgt != undefinedgt<T> && cgt != undefinedgt<T> && tgt != cgt)
        throw ics::TemplateFunctionError("LinkedPriorityQueue::Iterable constructor: both specified and different");
    for(const T& elem : i)
        enqueue(elem);
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::empty() const {
    return used == 0;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
int LinkedPriorityQueue<T,tgt>::size() const {
    return used;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
T& LinkedPriorityQueue<T,tgt>::peek () const {
    if(this->empty())
        throw ics::EmptyError("LinkedPriorityQueue::peek");
    return front->next->value;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::string LinkedPriorityQueue<T,tgt>::str() const {
    std::ostringstream answer;
    answer << "linked_priority_queue[HEADER";
    for(LinkedPriorityQueue<T,tgt>::LN* node = front; node->next != nullptr; node = node->next){
        answer << "->" << node->next->value;
    }
    answer << "](used=" << used << ",front=" << front << ",mod_count=" << mod_count << ")";
    return answer.str();
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class T, bool (*tgt)(const T& a, const T& b)>
int LinkedPriorityQueue<T,tgt>::enqueue(const T& element) {
    ++used;
    ++mod_count;
    LinkedPriorityQueue<T,tgt>::LN* node = new LinkedPriorityQueue<T,tgt>::LN(element);
    LinkedPriorityQueue<T,tgt>::LN *temp = front;
    for( ; temp->next!=nullptr; temp = temp->next){
        if(gt(element,temp->next->value)) {
            node->next = temp->next;
            break;
        }
    }
    temp->next = node;
    return 1;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
T LinkedPriorityQueue<T,tgt>::dequeue() {
    if(this->empty())
        throw ics::EmptyError("LinkedPriorityQueue::dequeue");
    T returnVal = front->next->value;
    LinkedPriorityQueue<T,tgt>::LN* node = front->next;
    front->next = front->next->next;
    delete node;
    --used;
    ++mod_count;
    return returnVal;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
void LinkedPriorityQueue<T,tgt>::clear() {
    delete_list(front);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
template <class Iterable>
int LinkedPriorityQueue<T,tgt>::enqueue_all (const Iterable& i) {
    int count = 0;
    for(auto const &m : i){
        count += enqueue(m);
    }
    return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>& LinkedPriorityQueue<T,tgt>::operator = (const LinkedPriorityQueue<T,tgt>& rhs) {
    if(this == &rhs)
        return *this;
    if(gt != rhs.gt) {
        gt = rhs.gt;
        delete_list(front);
        for (LinkedPriorityQueue<T, tgt>::Iterator i = rhs.begin(); i != rhs.end(); ++i)
            enqueue(*i);
    }
    else {
        LinkedPriorityQueue<T, tgt>::LN *ref = rhs.front;
        mod_count += rhs.used;
        int difference = used - rhs.used;
        for (int i = 0; i < difference; i++)
            dequeue();
        for (LinkedPriorityQueue<T, tgt>::LN **t = &front; (*t) != nullptr; t = &((*t)->next)) {
            if (ref != nullptr) {
                if((*t) != front)
                    (*t)->value = ref->value;
                ref = ref->next;
            }
        }
        for (; ref != nullptr; ref = ref->next)
            enqueue(ref->value);
    }
    return *this;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::operator == (const LinkedPriorityQueue<T,tgt>& rhs) const {
    if(this == &rhs)
        return true;
    if(used != rhs.size())
        return false;
    if(gt != rhs.gt)
        return false;
    LinkedPriorityQueue<T,tgt>::Iterator selfIterator = this->begin();
    for(LinkedPriorityQueue<T,tgt>::Iterator i = rhs.begin(); i != rhs.end(); ++i){
        if(*i != *selfIterator)
            return false;
        ++selfIterator;
    }
    return true;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::operator != (const LinkedPriorityQueue<T,tgt>& rhs) const {
    return !(*this == rhs);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::ostream& operator << (std::ostream& outs, const LinkedPriorityQueue<T,tgt>& pq) {
    outs << "priority_queue[";
    int count = 0;
    int max = pq.size() - 1;
    if(!(pq.empty())){
        std::stringstream sstream;
        for(const T& item : pq){
            sstream << item;
            if(count < max)
                sstream<< ",";
            count++;
        }
        std::string ordered = sstream.str();
        for(int i = ordered.length() - 1; i >= 0; i--)
            outs << ordered[i];
    }
    outs << "]:highest";
    return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::begin () const -> LinkedPriorityQueue<T,tgt>::Iterator {
    return Iterator(const_cast<LinkedPriorityQueue<T,tgt>*>(this),this->front->next);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::end () const -> LinkedPriorityQueue<T,tgt>::Iterator {
    return Iterator(const_cast<LinkedPriorityQueue<T,tgt>*>(this), nullptr);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T, bool (*tgt)(const T& a, const T& b)>
void LinkedPriorityQueue<T,tgt>::delete_list(LN*& front) {
    while(front->next != nullptr){
        LinkedPriorityQueue<T,tgt>::LN* temp = front->next;
        front->next = front->next->next;
        delete temp;
        ++mod_count;
        --used;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::Iterator::Iterator(LinkedPriorityQueue<T,tgt>* iterate_over, LN* initial)
{
    ref_pq = iterate_over;
    expected_mod_count = ref_pq->mod_count;
    prev = ref_pq->front;
    current = initial;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
LinkedPriorityQueue<T,tgt>::Iterator::~Iterator()
{}


template<class T, bool (*tgt)(const T& a, const T& b)>
T LinkedPriorityQueue<T,tgt>::Iterator::erase() {
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("LinkedPriorityQueue::Iterator::erase");
    if(!can_erase)
        throw ics::CannotEraseError("LinkedPriorityQueue::Iterator::erase Iterator cursor already erase");
    if(current == nullptr)
        throw ics::CannotEraseError("LinkedPriorityQueue::Iterator::erase Iterator cursor beyond data structure");
    can_erase = false;
    LinkedPriorityQueue<T,tgt>::LN* node = current;
    T to_return = current->value;
    if(prev == ref_pq->front){
        current = current->next;
        prev->next = current;
        //ref_pq->front->next = current
    }
    else{
        prev->next = current->next;
        current = prev->next;
    }
    delete node;
    ref_pq->used--;
    ref_pq->mod_count = ++expected_mod_count;
    return to_return;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::string LinkedPriorityQueue<T,tgt>::Iterator::str() const {
    std::ostringstream answer;
    answer << ref_pq->str() << "(current=" << current << ",expected_mod_count=" << expected_mod_count
           << ",can_erase=" << can_erase << ")";
    return answer.str();
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::Iterator::operator ++ () -> LinkedPriorityQueue<T,tgt>::Iterator& {
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ++");
    if(current == nullptr)
        return *this;
    if(can_erase){
        prev = current;
        current = current->next;
    }
    else
        can_erase = true;
    return *this;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto LinkedPriorityQueue<T,tgt>::Iterator::operator ++ (int) -> LinkedPriorityQueue<T,tgt>::Iterator {
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ++(int)");
    if(current == nullptr)
        return *this;
    Iterator to_return(*this);
    if(can_erase){
        prev = current;
        current = current->next;
    }
    else
        can_erase = true;
    return to_return;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::Iterator::operator == (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if (rhsASI == 0)
        throw ics::IteratorTypeError("LinkedPriorityQueue::Iterator::operator ==");
    if (expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator ==");
    if (ref_pq != rhsASI->ref_pq)
        throw ics::ComparingDifferentIteratorsError("LinkedPriorityQueue::Iterator::operator ==");
    return current == rhsASI->current;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool LinkedPriorityQueue<T,tgt>::Iterator::operator != (const LinkedPriorityQueue<T,tgt>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if (rhsASI == 0)
        throw ics::IteratorTypeError("LinkedPriorityQueue::Iterator::operator !=");
    if (expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("LinkedPriorityQueue::Iterator::operator !=");
    if (ref_pq != rhsASI->ref_pq)
        throw ics::ComparingDifferentIteratorsError("LinkedPriorityQueue::Iterator::operator !=");
    return current != rhsASI->current;
}

template<class T, bool (*tgt)(const T& a, const T& b)>
T& LinkedPriorityQueue<T,tgt>::Iterator::operator *() const {
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("LinkedPriorityQueue::Iterator operator *");
    if(!can_erase || current == nullptr)
        throw ics::IteratorPositionIllegal("LinkedPriorityQueue::Iterator operator *: Iterator illegal");
    return current->value;
}

template<class T, bool (*tgt)(const T& a, const T& b)>
T* LinkedPriorityQueue<T,tgt>::Iterator::operator ->() const {
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("LinkedPriorityQueue::Iterator operator ->");
    if(!can_erase || current == ref_pq->front || current == nullptr)
        throw ics::IteratorPositionIllegal("LinkedPriorityQueue::Iterator operator ->: Iterator illegal");
    return &current->value;
}
}

#endif /* LINKED_PRIORITY_QUEUE_HPP_ */
