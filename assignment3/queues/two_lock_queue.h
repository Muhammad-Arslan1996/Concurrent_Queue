#include "../common/allocator.h"
#include <mutex>

template <class T>
struct Node
{
  T value;
  Node<T>* next;
};

template <class T>
class TwoLockQueue
{
    CustomAllocator my_allocator_;
    std::mutex h_lock, t_lock;
    Node<T>* head;
    Node<T>* tail;

public:
    TwoLockQueue() : my_allocator_()
    {
        std::cout << "Using TwoLockQueue\n";
    }

    void initQueue(long t_my_allocator_size){
        std::cout << "Using Allocator\n";
        my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
        Node<T>* newNode = (Node<T>*)my_allocator_.newNode();
        newNode->next = NULL;
        head = tail = newNode;
        my_allocator_.freeNode(newNode);
    }

    void enqueue(T value)
    {
      Node<T>* node = (Node<T>* )my_allocator_.newNode();
      node->value = value;
      node->next = NULL;
      t_lock.lock();
      tail->next = node;
      tail = node;
      t_lock.unlock();
    }

    bool dequeue(T *value)
    {
      bool ret_value = false;
      h_lock.lock();
      Node<T>* node = head;
      Node<T>* new_head = head->next;
      if(new_head == NULL){
          // Queue is empty
          h_lock.unlock();
          return ret_value;
      }
      *value = new_head->value;
      head = new_head;
      ret_value = true;
      h_lock.unlock();
      my_allocator_.freeNode(node);
      return ret_value;
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }
};
