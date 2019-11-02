#include "../common/allocator.h"
#include "../common/utils.h"

#define LFENCE asm volatile("lfence" : : : "memory")
#define SFENCE asm volatile("sfence" : : : "memory")

template<class P>
struct pointer_t {
    P* ptr;

    P* address(){
        P* address = (P*)((uintptr_t) ptr & (uintptr_t) 0x0000ffffffffffff);
        return address;
    }
    long count(){
        long count = (long)((uintptr_t) ptr & (uintptr_t) 0xffff000000000000);
        return count >> 48;

    }
};

template <class T>
class Node
{
public:
  T value;
  pointer_t<Node<T>> next;
};

template <class T>
class NonBlockingQueue
{
  pointer_t<Node<T>> q_head;
  pointer_t<Node<T>> q_tail;
  CustomAllocator my_allocator_;
public:
    NonBlockingQueue() : my_allocator_()
    {
        std::cout << "Using NonBlockingQueue\n";
    }

    void initQueue(long t_my_allocator_size){
        std::cout << "Using Allocator\n";
        my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
        Node<T>* newNode = (Node<T>*)my_allocator_.newNode();
        newNode->next.ptr = NULL;
        q_head.ptr = q_tail.ptr = newNode;
        my_allocator_.freeNode(newNode);
    }
    pointer_t<Node<T>> incrementAndGetAddress(pointer_t<Node<T>> n1, pointer_t<Node<T>> n2){
      pointer_t<Node<T>> address;
      address.ptr = (Node<T> *)(((uintptr_t) (n2.count()+1) << 48) | (uintptr_t) n1.address());
      return address;
    }

    void enqueue(T value)
    {
      pointer_t<Node<T>> tail;
      pointer_t<Node<T>> next;
      Node<T>* node = (Node<T>* )my_allocator_.newNode();
      node->value = value;
      node->next.ptr = NULL;
      pointer_t<Node<T>> newNode;
      newNode.ptr = node;
      SFENCE;
      while(true) {
          tail = q_tail;
          LFENCE;
          next = tail.address()->next;
          LFENCE;
          if (tail.address() == q_tail.address()){
              if (next.address() == NULL) {
                  if(CAS(&tail.address()->next, next, incrementAndGetAddress(newNode, next)))
                      break;
              }
              else
                  CAS(&q_tail, tail, incrementAndGetAddress(next, tail));	// ELABEL
          }
      }
      SFENCE;
      CAS(&q_tail, tail, incrementAndGetAddress(newNode, tail));
    }
    bool dequeue(T *value)
    {
      pointer_t<Node<T>> head;
      pointer_t<Node<T>> tail;
      pointer_t<Node<T>> next;
      bool ret_value = false;
      while(true){
        head = q_head;
        LFENCE;
        tail = q_tail;
        LFENCE;
        next = head.address()->next;
        LFENCE;
        if (head.address() == q_head.address()) {
          if(head.address() == tail.address()) {
            if(next.address() == NULL)
              return ret_value;
            CAS(&q_tail, tail, incrementAndGetAddress(next, tail));	//DLABEL
            }
            else {
              *value = next.address()->value;
              if(CAS(&q_head, head, incrementAndGetAddress(next, head)))
                break;
                }
            }
        }
        my_allocator_.freeNode(head.address());
        ret_value = true;
        return ret_value;
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }

};
