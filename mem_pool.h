#ifndef _WGTCC_MEM_POOL_H_
#define _WGTCC_MEM_POOL_H_

#include <cstddef>
#include <vector>


class MemPool
{
public:
  MemPool(void): allocated_(0) {}
  
  virtual ~MemPool(void) {}
  
  MemPool(const MemPool& other) = delete;
  
  MemPool& operator=(const MemPool& other) = delete;

  virtual void* Alloc(void) = 0;
  
  virtual void Free(void* addr) = 0;
  
  virtual void Clear(void) = 0;

protected:
  size_t allocated_;
};


template <class T>
class MemPoolImp: public MemPool
{
public:
  MemPoolImp(void) : root_(nullptr) {}
  
  virtual ~MemPoolImp(void) {}

  MemPoolImp(const MemPool& other) = delete;
  
  MemPoolImp& operator=(MemPool& other) = delete;

  virtual void* Alloc(void);
  
  virtual void Free(void* addr);
  
  virtual void Clear(void);

private:
  enum {
    COUNT = (4 * 1024) / sizeof(T)
  };
  
  union Chunk {
    Chunk* _next;
    char _mem[sizeof(T)];
  };
  
  struct Block {
    Block(void) {
      for (size_t i = 0; i < COUNT - 1; i++)
        chunks_[i]._next = &chunks_[i+1];
      chunks_[COUNT-1]._next = nullptr;
    }
    Chunk chunks_[COUNT];
  };

  std::vector<Block*> blocks_;
  Chunk* root_;
};


template <class T>
void* MemPoolImp<T>::Alloc(void)
{
  if (nullptr == root_) { //空间不够，需要分配空间
    auto block = new Block();
    root_ = block->chunks_;
    //如果blocks实现为std::list, 那么push_back实际的overhead更大
    //这也表明，即使我们不需要随机访问功能(那么std::vector的拷贝是一种overhead)，
    //仍然倾向于使用std::vector，
    //当然std::vector的指数级capacity增长会造成内存浪费。
    blocks_.push_back(block);
  }
  
  auto ret = root_;
  root_ = root_->_next;

  ++allocated_;
  return ret;
}


template <class T>
void MemPoolImp<T>::Free(void* addr)
{
  if (nullptr == addr) 
    return;

  auto chunk = static_cast<Chunk*>(addr);
  chunk->_next = root_;
  root_ = chunk;

  --allocated_;
}


template <class T>
void MemPoolImp<T>::Clear(void)
{
  for (auto block: blocks_)
    delete block;

  blocks_.resize(0);
  root_ = nullptr;
  allocated_ = 0;
}

#endif
