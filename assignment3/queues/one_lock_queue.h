#include "../common/allocator.h"
#include <mutex>

template <class T>
struct Node{
  T value;
  Node<T>* next;
};

template <class T>
class OneLockQueue
{
    CustomAllocator my_allocator_;
    std::mutex l1;
    Node<T>* head;
    Node<T>* tail;
public:
    OneLockQueue() : my_allocator_()
    {
        std::cout << "Using OneLockQueue\n";
    }

    void initQueue(long t_my_allocator_size){
        std::cout << "Using Allocator\n";
        my_allocator_.initialize(t_my_allocator_size, sizeof(Node<T>));
        Node<T>* newNode = (Node<T>*)my_allocator_.newNode();
        newNode->next = NULL;
        head = tail = newNode;
        my_allocator_.freeNode(newNode);
    }

    void enqueue(T value){
      Node<T>* node = (Node<T>* )my_allocator_.newNode();
      node->value = value;
      node->next = NULL;
      l1.lock();
      tail->next = node;
      tail = node;
      l1.unlock();
    }

    bool dequeue(T *value)
    {
      bool ret_value = false;
      l1.lock();
      Node<T>* node = head;
      Node<T>* new_head = head->next;
      if(new_head == NULL){
          // Queue is empty
          l1.unlock();
          return ret_value;
      }
      *value = new_head->value;
      head = new_head;
      ret_value = true;
      l1.unlock();
      my_allocator_.freeNode(node);
      return ret_value;
    }

    void cleanup()
    {
        my_allocator_.cleanup();
    }
};
