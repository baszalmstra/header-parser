// Run with: header-parser example1.h -c TCLASS -e TENUM -f TFUNC -p TPROPERTY

#include <vector>

namespace test 
{
	TCLASS()
	class Foo : public Bar 
	{
		TPROPERTY()
		int ThisIsAPrivateProperty;
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

TCLASS()
struct Test {
	TPROPERTY()
	int ThisIsAPublicArray1[1];
	
	TPROPERTY()
	int ThisIsAPublicArray2[CONST];

	TPROPERTY()
	int ThisIsAPublicProperty;
private:
	TPROPERTY()
	int ThisIsAPrivateProperty;
};