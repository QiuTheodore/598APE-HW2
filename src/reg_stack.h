
#pragma once

namespace genetic {

/**
 * @brief Fixed-capacity stack for AST evaluation.
 *
 * Optimized to provide O(1) push and pop operations
 * using direct array indexing.
 */
template <typename DataT, int MaxSize>
struct stack {
  explicit stack() : elements_(0) {
    for (int i = 0; i < MaxSize; ++i) {
      regs_[i] = DataT(0);
    }
  }

  /** Checks whether the stack is empty. */
  bool empty() const {
    return elements_ == 0;
  }

  /** Returns the number of elements. */
  int size() const {
    return elements_;
  }

  /** Checks whether the stack is full. */
  bool full() const {
    return elements_ == MaxSize;
  }

  /**
   * @brief Push a value onto the stack.
   *
   * Optimization:
   * Use direct indexing instead of scanning the array.
   * Preserve original behavior when the stack is full.
   */
  void push(DataT val) {
    if (elements_ < MaxSize) {
      regs_[elements_] = val;
      ++elements_;
    }
  }

  /**
   * @brief Pop the top value.
   *
   * Optimization:
   * Use direct indexing instead of searching for the top.
   * Preserve original behavior when the stack is empty.
   */
  DataT pop() {
    if (elements_ > 0) {
      --elements_;
      return regs_[elements_];
    }

    return DataT(0);
  }

private:
  int elements_;
  DataT regs_[MaxSize];
};

} // namespace genetic
