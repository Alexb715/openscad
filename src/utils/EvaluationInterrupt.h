#pragma once

#include <atomic>

#include "utils/exceptions.h"
#include "utils/printutils.h"

/*
 * Cooperative interruption of SCAD evaluation (instantiation).
 *
 * An embedding host (e.g. a long-lived engine thread with a watchdog) sets
 * evaluation_interrupt_requested from another thread; the evaluator checks it at
 * module/function calls, tail calls and every for/comprehension step and unwinds with
 * EvaluationInterruptedException, which SourceFile::instantiate() handles like any other
 * EvaluationException. The flag is never set by OpenSCAD itself.
 */
inline std::atomic<bool> evaluation_interrupt_requested{false};

class EvaluationInterruptedException : public EvaluationException
{
public:
  EvaluationInterruptedException() : EvaluationException("Evaluation interrupted")
  {
    traceDepth = 0;
    tail_msgs.set_capacity(0);
  }
  // Runs before ~EvaluationException: catch sites decrement traceDepth while unwinding, which
  // would otherwise print an "Excluding N frames" trace for an interrupt.
  ~EvaluationInterruptedException()
  {
    traceDepth = 0;
    tail_msgs.clear();
  }
};

inline void check_evaluation_interrupt()
{
  if (__builtin_expect(evaluation_interrupt_requested.load(std::memory_order_relaxed), 0)) {
    // Skip eager parameter stringification in UserModule's trace path while unwinding.
    OpenSCAD::traceUsermoduleParameters = false;
    throw EvaluationInterruptedException();
  }
}
