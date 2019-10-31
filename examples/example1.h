// Run with: header-parser example1.h -c TCLASS -e TENUM -f TFUNC -p TPROPERTY

#include <vector>

namespace test 
{
	TCLASS()
	class Foo : public Bar 
	{
	protected:
		TFUNC(Arg=3)
		bool ProtectedFunction(std::vector<int> args) const;

	public:
    TENUM()
      enum Enum
    {
      FirstValue,
      SecondValue = 3
    };

	public:
		TPROPERTY()
		int ThisIsAProperty;
  };

	TCLASS()
	template<typename T, typename Base=Foo>
	class TemplatedFoo : public Base
  {

	};
}