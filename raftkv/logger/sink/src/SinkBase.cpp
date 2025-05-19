#include "SinkBase.h"

namespace WW
{

SinkBase::SinkBase(const std::string & pattern)
    : formatter(std::make_shared<DefaultFormatter>(pattern))
{
}

SinkBase::SinkBase(std::shared_ptr<FormatterBase> formatter)
    : formatter(std::move(formatter))
{
}

void SinkBase::setFormatter(std::shared_ptr<FormatterBase> formatter)
{
    this->formatter = std::move(formatter);
}

void SinkBase::setFormatter(const std::string & pattern)
{
    this->formatter = std::make_shared<DefaultFormatter>(pattern);
}

} // namespace WW
