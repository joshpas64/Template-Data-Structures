// Submitter : jpascasc(Pascascio, Joshua)

#ifndef LINKED_QUEUE_HPP_
#define LINKED_QUEUE_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include "ics_exceptions.hpp"


namespace ics {


template<class T> class LinkedQueue {
  public:
    //Destructor/Constructors
    ~LinkedQueue();

    LinkedQueue          ();
    LinkedQueue          (const LinkedQueue<T>& to_copy);
    explicit LinkedQueue (const std::initializer_list<T>& il);

    //Iterable class must support "for-each" loop: .begin()/.end() and prefix ++ on returned result
    template <class Iterable>
    explicit LinkedQueue (const Iterable& i);


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
    LinkedQueue<T>& operator = (const LinkedQueue<T>& rhs);
    bool operator == (const LinkedQueue<T>& rhs) const;
    bool operator != (const LinkedQueue<T>& rhs) const;

    template<class T2>
    friend std::ostream& operator << (std::ostream& outs, const LinkedQueue<T2>& q);



  private:
    class LN;

  public:
    class Iterator {
      public:
        //Private constructor called in begin/end, which are friends of LinkedQueue<T>
        ~Iterator();
        T           erase();
        std::string str  () const;
        LinkedQueue<T>::Iterator& operator ++ ();
        LinkedQueue<T>::Iterator  operator ++ (int);
        bool operator == (const LinkedQueue<T>::Iterator& rhs) const;
        bool operator != (const LinkedQueue<T>::Iterator& rhs) const;
        T& operator *  () const;
        T* operator -> () const;
        friend std::ostream& operator << (std::ostream& outs, const LinkedQueue<T>::Iterator& i) {
          outs << i.str(); //Use the same meaning as the debugging .str() method
          return outs;
        }
        friend Iterator LinkedQueue<T>::begin () const;
        friend Iterator LinkedQueue<T>::end   () const;

      private:
        //If can_erase is false, current indexes the "next" value (must ++ to reach it)
        LN*             prev = nullptr;  //if nullptr, current at front of list
        LN*             current;         //current == prev->next (if prev != nullptr)
        LinkedQueue<T>* ref_queue;
        int             expected_mod_count;
        bool            can_erase = true;

        //Called in friends begin/end
        Iterator(LinkedQueue<T>* iterate_over, LN* initial);
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


    LN* front     =  nullptr;
    LN* rear      =  nullptr;
    int used      =  0;            //Cache count of nodes in linked list
    int mod_count =  0;            //Alllows sensing concurrent modification

    //Helper methods
    void delete_list(LN*& front);  //Deallocate all LNs, and set front's argument to nullptr;
};





////////////////////////////////////////////////////////////////////////////////
//
//LinkedQueue class and related definitions

//Destructor/Constructors

template<class T>
LinkedQueue<T>::~LinkedQueue() {
    delete_list(front);
    delete front;
    delete rear;
}


template<class T>
LinkedQueue<T>::LinkedQueue() {
}


template<class T>
LinkedQueue<T>::LinkedQueue(const LinkedQueue<T>& to_copy) {
    for(LinkedQueue<T>::LN* node = to_copy.front; node != nullptr; node = node->next){
        enqueue(node->value);
    }
}


template<class T>
LinkedQueue<T>::LinkedQueue(const std::initializer_list<T>& il) {
    for(const T& elem : il)
        enqueue(elem);
}


template<class T>
template<class Iterable>
LinkedQueue<T>::LinkedQueue(const Iterable& i) {
    for(const auto &t : i){
        enqueue(t);
    }
}


////////////////////////////////////////////////////////////////////////////////
//
//Queries

template<class T>
bool LinkedQueue<T>::empty() const {
    return (used == 0);
}


template<class T>
int LinkedQueue<T>::size() const {
    return used;
}


template<class T>
T& LinkedQueue<T>::peek () const {
    if(empty())
        throw ics::EmptyError("LinkedQueue::peek");
    return front->value;
}


template<class T>
std::string LinkedQueue<T>::str() const {
    std::ostringstream answer;
    answer << "LinkedQueue[";
    if(used != 0){
        for(LinkedQueue<T>::LN* node = front; node != nullptr; node = node->next){
            answer << node->value;
            if(node->next != nullptr)
                answer << "->";
        }
    }
    answer << "]";
    answer << "(used=" << used << ",front=";
    answer << front << ",rear=" << rear;
    answer << ",mod_count=" << mod_count << ")";
    return answer.str();
}


////////////////////////////////////////////////////////////////////////////////
//
//Commands

template<class T>
int LinkedQueue<T>::enqueue(const T& element) {
    LinkedQueue<T>::LN *node = new LinkedQueue<T>::LN(element);
    if(front == nullptr){
       front = rear = node;
    }
    else if(front == rear && used == 1){
        front->next = node;
        rear = node;
    }
    else{
        rear->next = node;
        rear = node;
    }
    ++mod_count;
    ++used;
    return 1;
}


template<class T>
T LinkedQueue<T>::dequeue() {
    if(empty()){
        throw ics::EmptyError("LinkedQueue::dequeue");
    }
    T var = front->value;
    LinkedQueue<T>::LN *temp = front;
    front = front->next;
    if(front == nullptr)
        rear = nullptr;
    mod_count++;
    used--;
    delete temp;
    return var;
}


template<class T>
void LinkedQueue<T>::clear() {
    delete_list(front);
}


template<class T>
template<class Iterable>
int LinkedQueue<T>::enqueue_all(const Iterable& i) {
    int count = 0;
    for(const T &v : i)
        count += enqueue(v);
    return count;
}


////////////////////////////////////////////////////////////////////////////////
//
//Operators

template<class T>
LinkedQueue<T>& LinkedQueue<T>::operator = (const LinkedQueue<T>& rhs) { // Essentially like a move, NOT COPY constructor
    if(this == &rhs)
        return *this;
    LinkedQueue<T>::LN* test = rhs.front;
    mod_count += rhs.used;
    int difference = used - rhs.used;
    for(int i = 0; i < difference; i++)
        dequeue();
    for( LinkedQueue<T>::LN** t = &front; (*t) != nullptr; t = &((*t)->next)){
        if(test != nullptr){
            (*t)->value = test->value;
            test = test->next;
        }
    }
    for( ; test != nullptr; test = test->next)
        enqueue(test->value);
    return *this;
}
template<class T>
bool LinkedQueue<T>::operator == (const LinkedQueue<T>& rhs) const {
    if(this == &rhs)
        return true;
    if(used != rhs.size())
        return false;
    LinkedQueue<T>::Iterator selfIterator = this->begin();
    for(LinkedQueue<T>::Iterator i = rhs.begin(); i != rhs.end(); ++i){
        if(*i != *selfIterator)
            return false;
        ++selfIterator;
    }
    return true;
}


template<class T>
bool LinkedQueue<T>::operator != (const LinkedQueue<T>& rhs) const {
    return !(*this == rhs);
}


template<class T>
std::ostream& operator << (std::ostream& outs, const LinkedQueue<T>& q) {
    outs << "queue[";
    int len = q.size() - 1;
    int index = 0;
    for(auto &node : q){
        outs << node;
        if(index < len)
            outs << ",";
        index++;
    }
    outs << "]:rear";
    return outs;
}


////////////////////////////////////////////////////////////////////////////////
//
//Iterator constructors

template<class T>
auto LinkedQueue<T>::begin () const -> LinkedQueue<T>::Iterator {
    return Iterator(const_cast<LinkedQueue<T>*>(this), this->front);
}

template<class T>
auto LinkedQueue<T>::end () const -> LinkedQueue<T>::Iterator {
    return Iterator(const_cast<LinkedQueue<T>*>(this),nullptr);
}


////////////////////////////////////////////////////////////////////////////////
//
//Private helper methods

template<class T>
void LinkedQueue<T>::delete_list(LN*& front) {
    while(front != nullptr){
        LinkedQueue<T>::LN *temp = front;
        front = front->next;
        delete temp;
        ++mod_count;
    }
    //mod_count++;
    //delete rear;
    used = 0;
    rear = front = nullptr;
}





////////////////////////////////////////////////////////////////////////////////
//
//Iterator class definitions

template<class T>
LinkedQueue<T>::Iterator::Iterator(LinkedQueue<T>* iterate_over, LN* initial)
{
    ref_queue = iterate_over;
    expected_mod_count = ref_queue->mod_count;
    current = initial;
}


template<class T>
LinkedQueue<T>::Iterator::~Iterator()
{}


template<class T>
T LinkedQueue<T>::Iterator::erase() {
    if(expected_mod_count != ref_queue->mod_count)
        throw ics::ConcurrentModificationError("LinkedQueue::Iterator erase");
    if(current == nullptr)
        throw ics::CannotEraseError("LinkedQueue::Iterator::erase Iterator cursor beyond data structure");
    if(!can_erase)
        throw ics::CannotEraseError("LinkedQueue::Iterator::erase Iterator cursor already erased");
    LinkedQueue<T>::LN* node = current;
    T returnVal = current->value;
    can_erase = false;
    if(prev != nullptr){
        prev->next = current->next;
        current = prev->next;
    }
    else{
        current = current->next;
        ref_queue->front = current;
    }
    delete node;
    ref_queue->used--;
    ref_queue->mod_count = ++expected_mod_count;
    return returnVal;
}


template<class T>
std::string LinkedQueue<T>::Iterator::str() const {
    std::ostringstream answer;
    answer << ref_queue->str() << "(current=" << current << ",expected_mod_count=" << expected_mod_count << ",can_erase=" << can_erase << ")";
    return answer.str();
}


template<class T>
auto LinkedQueue<T>::Iterator::operator ++ () -> LinkedQueue<T>::Iterator& {
    if(expected_mod_count != ref_queue->mod_count)
        throw ics::ConcurrentModificationError("LinkedQueue::Iterator::operator ++");
    if(current == ref_queue->rear->next)
        return *this;
    if(can_erase){
        prev = current;
        current = current->next;
    }
    else
        can_erase = true;
    return *this;
}


template<class T>
auto LinkedQueue<T>::Iterator::operator ++ (int) -> LinkedQueue<T>::Iterator {
    if(expected_mod_count != ref_queue->mod_count)
        throw ics::ConcurrentModificationError("LinkedQueue::Iterator::operator ++(int)");
    if(current == ref_queue->rear->next)
        return *this;
    Iterator i_return(*this);
    if(can_erase){
        prev = current;
        current = current->next;
    }
    else
        can_erase = true;
    return i_return;
}


template<class T>
bool LinkedQueue<T>::Iterator::operator == (const LinkedQueue<T>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if (rhsASI == 0)
        throw ics::IteratorTypeError("LinkedQueue::Iterator::operator ==");
    if (expected_mod_count != ref_queue->mod_count)
        throw ics::ConcurrentModificationError("LinkedQueue::Iterator::operator ==");
    if (ref_queue != rhsASI->ref_queue)
        throw ics::ComparingDifferentIteratorsError("LinkedQueue::Iterator::operator ==");
    return current == rhsASI->current;
}


template<class T>
bool LinkedQueue<T>::Iterator::operator != (const LinkedQueue<T>::Iterator& rhs) const {
    const Iterator* rhsASI = dynamic_cast<const Iterator*>(&rhs);
    if (rhsASI == 0)
        throw ics::IteratorTypeError("LinkedQueue::Iterator::operator !=");
    if (expected_mod_count != ref_queue->mod_count)
        throw ics::ConcurrentModificationError("LinkedQueue::Iterator::operator !=");
    if (ref_queue != rhsASI->ref_queue)
        throw ics::ComparingDifferentIteratorsError("LinkedQueue::Iterator::operator !=");
    return current != rhsASI->current;
}


template<class T>
T& LinkedQueue<T>::Iterator::operator *() const {
    if(expected_mod_count != ref_queue->mod_count)
        throw ics::ConcurrentModificationError("LinkedQueue::Iterator::operator *");
    if(!can_erase || current == nullptr)
        throw ics::IteratorPositionIllegal("LinkedQueue::Iterator::operator *");
    return current->value;
}


template<class T>
T* LinkedQueue<T>::Iterator::operator ->() const {
    if(expected_mod_count != ref_queue->mod_count)
        throw ics::ConcurrentModificationError("LinkedQueue::Iterator ->");
    if(!can_erase || current == nullptr)
        throw ics::IteratorPositionIllegal("LinkedQueue::Iterator ->");
    return &current->value;
}


}

#endif /* LINKED_QUEUE_HPP_ */
