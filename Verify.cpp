#include "Verify.h"
#include "utils.h"

VerifyPlan::VerifyPlan(Plan * const input, RowSize const size, bool descending)
    : _input(input), _size(size), _descending(descending)
{
    TRACE(false);
} // VerifyPlan::VerifyPlan

VerifyPlan::~VerifyPlan()
{
    TRACE(false);
    // delete the input plan as in the chain deletion
    delete _input;
} // VerifyPlan::~VerifyPlan

Iterator * VerifyPlan::init() const
{
    TRACE(false);
    return new VerifyIterator(this);
} // VerifyPlan::init

VerifyIterator::VerifyIterator(VerifyPlan const * const plan)
    : _plan(plan), _input(_plan->_input->init()), _consumed(0), _produced(0),
        isSorted(true), hasDuplicates(false), _descending(_plan->_descending)
{
    TRACE(false);
    lastRow = nullptr;
} // VerifyIterator::VerifyIterator

VerifyIterator::~VerifyIterator()
{
    TRACE (false);
    free(lastRow);

    delete _input;

    #ifdef PRODUCTION
    string output = "Result is ";
    output += isSorted ? "sorted" : "not sorted";
    output += " ";
    output += _descending ? "descending" : "ascending";
    Trace::PrintTrace(OP_RESULT, VERIFY_RESULT, output);

    output = "Result has ";
    output += hasDuplicates ? "duplicates" : "no duplicates";
    Trace::PrintTrace(OP_RESULT, VERIFY_RESULT, output);
    #endif

    #if defined(VERBOSEL2) || defined(VERBOSEL1)
    traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
    #endif
} // VerifyIterator::~VerifyIterator

byte * VerifyIterator::next()
{
    TRACE (false);

    byte * received = _input->next();
    if (received == nullptr) {
        return nullptr;
    } else {
        ++ _consumed;

        #ifdef VERBOSEL2
        traceprintf ("#%llu consumed %s\n", _consumed, rowToString(received, _plan->_size).c_str());
        #endif

        if (lastRow != nullptr) {
            auto cmp = memcmp(lastRow, received, _plan->_size);
            if (_descending ? cmp < 0 : cmp > 0) {
                isSorted = false;

                #ifdef VERBOSEL1
                traceprintf ("!!!!!! Sort error: #%lu || %s #%lu || %s\n", (unsigned long) (_consumed - 1), rowToString(lastRow, _plan->_size).c_str(), (unsigned long) (_consumed), rowToString(received, _plan->_size).c_str());
                #endif
            }
            else if (cmp == 0) {
                hasDuplicates = true;

                #ifdef VERBOSEL2
                traceprintf ("#%lu has duplicates with value %s\n", (unsigned long) (_consumed), rowToString(lastRow, _plan->_size).c_str());
                #endif
            }
        } else {
            lastRow = (byte *) malloc(_plan->_size);
        }

        for (int i = 0; i < _plan->_size; ++i) {
            byte c = received[i];
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                continue;
            } else {
                throw std::invalid_argument("Invalid character in row " + rowToHexString(received, _plan->_size) + std::to_string(_consumed));
            }
        }

        // copy received to lastRow
        memcpy(lastRow, received, _plan->_size);
        
        ++ _produced;
        
        return received;
    }
} // VerifyIterator::next