#ifndef HEAP_PRIORITY_QUEUE_HPP_
#define HEAP_PRIORITY_QUEUE_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"
#include <utility>              //For std::swap function
#include "array_stack.hpp"      //See operator <<

// Submitter jpascasc(Pascascio,Joshua)
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
template<class T, bool (*tgt)(const T& a, const T& b) = undefinedgt<T>> class HeapPriorityQueue {
  public:
        typedef bool (*gtfunc) (const T& a, const T& b);
        //Destructor/Constructors
    ~HeapPriorityQueue();
    HeapPriorityQueue(bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);
    explicit HeapPriorityQueue(int initial_length, bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);
    HeapPriorityQueue(const HeapPriorityQueue<T,tgt>& to_copy, bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);
    explicit HeapPriorityQueue(const std::initializer_list<T>& il, bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit HeapPriorityQueue (const Iterable& i, bool (*cgt)(const T& a, const T& b) = undefinedgt<T>);


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
    HeapPriorityQueue<T,tgt>& operator = (const HeapPriorityQueue<T,tgt>& rhs);
    bool operator == (const HeapPriorityQueue<T,tgt>& rhs) const;
    bool operator != (const HeapPriorityQueue<T,tgt>& rhs) const;

    template<class T2, bool (*gt2)(const T2& a, const T2& b)>
    friend std::ostream& operator << (std::ostream& outs, const HeapPriorityQueue<T2,gt2>& pq);



    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of HeapPriorityQueue<T,tgt>
        ~Iterator();
        T           erase();
        std::string str  () const;
        HeapPriorityQueue<T,tgt>::Iterator& operator ++ ();
        HeapPriorityQueue<T,tgt>::Iterator  operator ++ (int);
        bool operator == (const HeapPriorityQueue<T,tgt>::Iterator& rhs) const;
        bool operator != (const HeapPriorityQueue<T,tgt>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const HeapPriorityQueue<T,tgt>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }

        friend Iterator HeapPriorityQueue<T,tgt>::begin () const;
        friend Iterator HeapPriorityQueue<T,tgt>::end   () const;

      private:
        //If can_erase is false, the value has been removed from "it" (++ does nothing)
        HeapPriorityQueue<T,tgt>  it;                 //copy of HPQ (from begin), to use as iterator via dequeue
        HeapPriorityQueue<T,tgt>* ref_pq;
        int                       expected_mod_count;
        bool                      can_erase = true;

        //Called in friends begin/end
        //These constructors have different initializers (see it(...) in first one)
        Iterator(HeapPriorityQueue<T,tgt>* iterate_over, bool from_begin);    // Called by begin
        Iterator(HeapPriorityQueue<T,tgt>* iterate_over);                     // Called by end
    };


    Iterator begin () const;
    Iterator end   () const;


  private:
    bool (*gt) (const T& a, const T& b); //The gt used by enqueue (from template or constructor)
    T*  pq;                              //Array stores a heap, so it uses the heap ordering property
    int length    = 0;                   //Physical length of array: must be >= .size()
    int used      = 0;                   //Amount of array used: invariant: 0 <= used <= length
    int mod_count = 0;                   //For sensing concurrent modification

    //Helper methods
    void ensure_length  (int new_length);
    int  left_child     (int i) const;         //Useful abstractions for heaps as arrays
    int  right_child    (int i) const;
    int  parent         (int i) const;
    bool is_root        (int i) const;
    bool in_heap        (int i) const;
    void percolate_up   (int i);
    void percolate_down (int i);
    void heapify        ();                   // Percolate down all value is array (from indexes used-1 to 0): O(N)
  };





////////////////////////////////////////////////////////////////////////////////
//
//HeapPriorityQueue class and related definitions

//Destructor/Constructors

template<class T, bool (*tgt)(const T& a, const T& b)>
HeapPriorityQueue<T,tgt>::~HeapPriorityQueue() {
    delete[] pq;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
HeapPriorityQueue<T,tgt>::HeapPriorityQueue(bool (*cgt)(const T& a, const T& b))
:gt( tgt != (gtfunc)undefinedgt<T> ? tgt : cgt)
{
    if(gt == (gtfunc)undefinedgt<T>)
        throw ics::TemplateFunctionError("HeapPriorityQueue::default constructor: neither specified");
    if(tgt != (gtfunc)undefinedgt<T> && cgt != (gtfunc)undefinedgt<T> && tgt != cgt)
        throw ics::TemplateFunctionError("HeapPriorityQueue::default constructor: both specified and different");
    pq = new T[length];
}


template<class T, bool (*tgt)(const T& a, const T& b)>
HeapPriorityQueue<T,tgt>::HeapPriorityQueue(int initial_length, bool (*cgt)(const T& a, const T& b))
:gt( (tgt != (gtfunc)undefinedgt<T>) ? tgt : cgt), length(initial_length)
{
    if(gt == (gtfunc)undefinedgt<T>)
        throw ics::TemplateFunctionError("HeapPriorityQueue::length constructor: neither specified");
    if( tgt != (gtfunc)undefinedgt<T> && cgt != (gtfunc)undefinedgt<T> && tgt != cgt)
        throw ics::TemplateFunctionError("HeapPriorityQueue::length constructor: both specified and different");
    if(length < 0)
        length = 0;
    pq = new T[length];
}


template<class T, bool (*tgt)(const T& a, const T& b)>
HeapPriorityQueue<T,tgt>::HeapPriorityQueue(const HeapPriorityQueue<T,tgt>& to_copy, bool (*cgt)(const T& a, const T& b))
:gt( tgt != (gtfunc)undefinedgt<T> ? tgt : cgt)
{
    if(gt == (gtfunc)undefinedgt<T>) {
        gt = to_copy.gt;
    }
    if(tgt != (gtfunc)undefinedgt<T> && cgt != (gtfunc)undefinedgt<T> && cgt != tgt)
        throw ics::TemplateFunctionError("HeapPriorityQueue::copy constructor: both specified and different");
    used = to_copy.used;
    length = to_copy.length;
    pq = new T[length];
    for(int i = 0; i < used; i++)
        pq[i] = to_copy.pq[i];
    if(gt != to_copy.gt) {
        heapify();
    }
}


template<class T, bool (*tgt)(const T& a, const T& b)>
HeapPriorityQueue<T,tgt>::HeapPriorityQueue(const std::initializer_list<T>& il, bool (*cgt)(const T& a, const T& b))
:gt(tgt != (gtfunc)undefinedgt<T> ? tgt : cgt),length(il.size()),used(il.size())
{
    if(gt == (gtfunc)undefinedgt<T>)
        throw ics::TemplateFunctionError("HeapPriorityQueue::initializer_list constructor: neither specified");
    if(tgt != (gtfunc)undefinedgt<T> && cgt != (gtfunc)undefinedgt<T> && tgt != cgt)
        throw ics::TemplateFunctionError("HeapPriorityQueue::initializer_list constructor: both specified and different");
    pq = new T[length];
    int count = 0;
    for(const T& elem : il){
        pq[count++] = elem;
    }
    heapify();
}


template<class T, bool (*tgt)(const T& a, const T& b)>
template<class Iterable>
HeapPriorityQueue<T,tgt>::HeapPriorityQueue(const Iterable& i, bool (*cgt)(const T& a, const T& b))
:gt( tgt != (gtfunc)undefinedgt<T> ? tgt : cgt),length(i.size()),used(i.size())
{
    if(gt == (gtfunc)undefinedgt<T>)
        throw ics::TemplateFunctionError("HeapPriorityQueue::Iterable constructor: neither specified");
    if(tgt != (gtfunc)undefinedgt<T> && cgt != (gtfunc)undefinedgt<T> && tgt != cgt)
        throw ics::TemplateFunctionError("HeapPriorityQueue::Iterable constructor: both specified and different");
    pq = new T[length];
    int count = 0;
    for(const T& elem : i)
        pq[count++] = elem;
    heapify();
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T, bool (*tgt)(const T& a, const T& b)>
bool HeapPriorityQueue<T,tgt>::empty() const {
    return (used == 0);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
int HeapPriorityQueue<T,tgt>::size() const {
    return used;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
T& HeapPriorityQueue<T,tgt>::peek () const {
    if(empty())
        throw ics::EmptyError("HeapPriorityQueue::peek");
    return pq[0];
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::string HeapPriorityQueue<T,tgt>::str() const {
    std::ostringstream result;
    result << "heappriorityqueue[";
    if(length != 0){
        std::stringstream backup;
        HeapPriorityQueue<T,tgt> copy(*this);
        int count = 0;
        int stop = used;
        int lstop = length - 1;
        for(int i = 0; i < length; i++){
            backup << i << ":";
            if(i < stop)
                backup << copy.dequeue();
            if(i < lstop)
                backup << ",";
        }
        result << backup.str();
    }
    result << "](length=" << length << ",used=" << used << ",mod_count=" << mod_count << "]";
    return result.str();
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class T, bool (*tgt)(const T& a, const T& b)>
int HeapPriorityQueue<T,tgt>::enqueue(const T& element) {
    this->ensure_length(used + 2);
    pq[used++] = element;
    percolate_up(used - 1);
    ++mod_count;
    return 1;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
T HeapPriorityQueue<T,tgt>::dequeue() {
    if(empty())
        throw ics::EmptyError("HeapPriorityQueue::dequeue");
    T val = pq[0];
    ++mod_count;
    pq[0] = pq[--used];
    if(!empty())
        percolate_down(0);
    return val;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
void HeapPriorityQueue<T,tgt>::clear() {
    ++mod_count;
    used = 0;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
template <class Iterable>
int HeapPriorityQueue<T,tgt>::enqueue_all(const Iterable& i) {
    int count = 0;
    for (const T& e : i)
        count += enqueue(e);
    return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T, bool (*tgt)(const T& a, const T& b)>
HeapPriorityQueue<T,tgt>& HeapPriorityQueue<T,tgt>::operator = (const HeapPriorityQueue<T,tgt>& rhs) {
    if(this == &rhs)
        return *this;
    if(gt == undefinedgt<T>)
        gt = rhs.gt;
    this->ensure_length(rhs.used);
    used = rhs.used;
    for(int i = 0; i < used; i++)
        pq[i] = rhs.pq[i];
    if(gt != rhs.gt){
        gt = rhs.gt;
        heapify();
    }
    ++mod_count;
    return *this;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool HeapPriorityQueue<T,tgt>::operator == (const HeapPriorityQueue<T,tgt>& rhs) const {
    if(this == &rhs)
        return true;
    if( used != rhs.size() || gt != rhs.gt)
        return false;
    HeapPriorityQueue<T,tgt>::Iterator iter = this->begin();
    for(HeapPriorityQueue<T,tgt>::Iterator i = rhs.begin(); i != rhs.end(); ++i){
        if(*i != *iter)
            return false;
        ++iter;
    }
    return true;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool HeapPriorityQueue<T,tgt>::operator != (const HeapPriorityQueue<T,tgt>& rhs) const {
    return !(*this == rhs);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::ostream& operator << (std::ostream& outs, const HeapPriorityQueue<T,tgt>& pq) {
    outs << "priority_queue[";
    int max = pq.size() - 1;
    int count = 0;
    if(!(pq.empty())){
        std::stringstream sstream;
        for(const T& item : pq){
            sstream << item;
            if(count < max)
                sstream << ",";
            count++;
        }
        std::string reverse = sstream.str();
        for(int i = (reverse.length() - 1) ; i >= 0; i--)
            outs << reverse[i];
    }
    outs << "]:highest";
    return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class T, bool (*tgt)(const T& a, const T& b)>
auto HeapPriorityQueue<T,tgt>::begin () const -> HeapPriorityQueue<T,tgt>::Iterator {
    return Iterator(const_cast<HeapPriorityQueue<T,tgt>*>(this),true);
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto HeapPriorityQueue<T,tgt>::end () const -> HeapPriorityQueue<T,tgt>::Iterator {
    return Iterator(const_cast<HeapPriorityQueue<T,tgt>*>(this));
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T, bool (*tgt)(const T& a, const T& b)>
void HeapPriorityQueue<T,tgt>::ensure_length(int new_length) {
    if(length >= new_length)
        return;
    T* oldpq = pq;
    length = std::max(new_length, 2 * length);
    pq = new T[length];
    for(int i = 0; i < used; i++)
        pq[i] = oldpq[i];
    delete[] oldpq;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
int HeapPriorityQueue<T,tgt>::left_child(int i) const
{
    return ((2 * i) + 1);
}

template<class T, bool (*tgt)(const T& a, const T& b)>
int HeapPriorityQueue<T,tgt>::right_child(int i) const
{
    return ((2 * i) + 2);
}

template<class T, bool (*tgt)(const T& a, const T& b)>
int HeapPriorityQueue<T,tgt>::parent(int i) const
{
    return ((i - 1) / 2);
}

template<class T, bool (*tgt)(const T& a, const T& b)>
bool HeapPriorityQueue<T,tgt>::is_root(int i) const
{
    if(empty())
        return false;
    return i == 0;
}

template<class T, bool (*tgt)(const T& a, const T& b)>
bool HeapPriorityQueue<T,tgt>::in_heap(int i) const
{
    return used != 0 && i < used;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
void HeapPriorityQueue<T,tgt>::percolate_up(int i) {
    for(int e = i; ( !(is_root(i)) && gt(pq[e],pq[parent(e)])); e = parent(e)){
        std::swap(pq[e],pq[parent(e)]);
        percolate_down(e);
    }
}


template<class T, bool (*tgt)(const T& a, const T& b)>
void HeapPriorityQueue<T,tgt>::percolate_down(int i) {
    int child, right;
    for(int e = i; (in_heap(e)) ; e = child){
        child = left_child(e);
        right = right_child(e);
        if(in_heap(right) && gt(pq[right],pq[child]))
            child = right;
        if(in_heap(child) && gt(pq[child],pq[e]))
            std::swap(pq[child],pq[e]);
        else
            break;
    }
}


template<class T, bool (*tgt)(const T& a, const T& b)>
void HeapPriorityQueue<T,tgt>::heapify() {
for (int i = used-1; i >= 0; --i)
  percolate_down(i);
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T, bool (*tgt)(const T& a, const T& b)>
HeapPriorityQueue<T,tgt>::Iterator::Iterator(HeapPriorityQueue<T,tgt>* iterate_over, bool tgt_nullptr)
{
    ref_pq = iterate_over;
    it = HeapPriorityQueue<T,tgt>(*ref_pq, ref_pq->gt);
    expected_mod_count = ref_pq->mod_count;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
HeapPriorityQueue<T,tgt>::Iterator::Iterator(HeapPriorityQueue<T,tgt>* iterate_over)
{
    ref_pq = iterate_over;
    it = HeapPriorityQueue<T,tgt>(0,ref_pq->gt);
    expected_mod_count = ref_pq->mod_count;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
HeapPriorityQueue<T,tgt>::Iterator::~Iterator()
{
}


template<class T, bool (*tgt)(const T& a, const T& b)>
T HeapPriorityQueue<T,tgt>::Iterator::erase() {
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("HeapPriorityQueue::Iterator::erase");
    if(!can_erase)
        throw ics::CannotEraseError("HeapPriorityQueue::Iterator::erase Iterator cursor has already been erased");
    if(it.size() == 0)
        throw ics::CannotEraseError("HeapPriorityQueue::Iterator::erase Iterator cursor beyond data structure");
    can_erase = false;
    T erasedVal = it.dequeue();
    int erasedIndex;
    for(int i = 0; i < ref_pq->used; i++){
        if(erasedVal == ref_pq->pq[i]){
            erasedIndex = i;
            break;
        }
    }
    ref_pq->pq[erasedIndex] = ref_pq->pq[--ref_pq->used];
    if(erasedIndex != 0 && ref_pq->gt(ref_pq->pq[erasedIndex],ref_pq->pq[ref_pq->parent(erasedIndex)]))
        ref_pq->percolate_up(erasedIndex);
    else
        ref_pq->percolate_down(erasedIndex);
    ref_pq->mod_count++;
    expected_mod_count = ref_pq->mod_count;
    return erasedVal;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
std::string HeapPriorityQueue<T,tgt>::Iterator::str() const {
    std::ostringstream answer;
    answer << "it=" << it.str() << "(cursor=" << it.size() << ",expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
    return answer.str();
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto HeapPriorityQueue<T,tgt>::Iterator::operator ++ () -> HeapPriorityQueue<T,tgt>::Iterator& {
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("HeapPriorityQueue::Iterator::operator ++()");
    if(it.size() == 0)
        return *this;
    if(can_erase)
        it.dequeue();
    else
        can_erase = true;
    return *this;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
auto HeapPriorityQueue<T,tgt>::Iterator::operator ++ (int) -> HeapPriorityQueue<T,tgt>::Iterator {
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("HeapPriorityQueue::Iterator::operator ++(int)");
    if(it.size() == 0)
        return *this;
    Iterator to_return(*this);
    if(can_erase)
        it.dequeue();
    else
        can_erase = true;
    return to_return;
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool HeapPriorityQueue<T,tgt>::Iterator::operator == (const HeapPriorityQueue<T,tgt>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if(rhsASI == 0)
        throw ics::IteratorTypeError("HeapPriorityQueue::Iterator::operator ==");
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("HeapPriorityQueue::Iterator::operator ==");
    if(ref_pq != rhsASI->ref_pq)
        throw ics::ComparingDifferentIteratorsError("HeapPriorityQueue::Iterator::operator ==");
    return it.size() == rhsASI->it.size();
}


template<class T, bool (*tgt)(const T& a, const T& b)>
bool HeapPriorityQueue<T,tgt>::Iterator::operator != (const HeapPriorityQueue<T,tgt>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if(rhsASI == 0)
        throw ics::IteratorTypeError("HeapPriorityQueue::Iterator::operator !=");
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("HeapPriorityQueue::Iterator::operator !=");
    if(ref_pq != rhsASI->ref_pq)
        throw ics::ComparingDifferentIteratorsError("HeapPriorityQueue::Iterator::operator !=");
    return it.size() != rhsASI->it.size();
}


template<class T, bool (*tgt)(const T& a, const T& b)>
T& HeapPriorityQueue<T,tgt>::Iterator::operator *() const {
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("HeapPriorityQueue::Iterator::operator *");
    if(!can_erase || it.size() == 0)
        throw ics::IteratorPositionIllegal("HeapPriorityQueue::Iterator::operator *: Iterator illegal");
    return it.peek();
}


template<class T, bool (*tgt)(const T& a, const T& b)>
T* HeapPriorityQueue<T,tgt>::Iterator::operator ->() const {
    if(expected_mod_count != ref_pq->mod_count)
        throw ics::ConcurrentModificationError("HeapPriorityQueue::Iterator::operator ->");
    if(!can_erase ||it.size() == 0)
        throw ics::IteratorPositionIllegal("HeapPriorityQueue::Iterator::operator ->: Iterator illegal");
    return &(it.peek());
}

}

#endif /* HEAP_PRIORITY_QUEUE_HPP_ */
