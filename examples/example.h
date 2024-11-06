// Run with: header-parser example.h -c TCLASS -e TENUM -f TFUNC -p TPROPERTY -q TCONSTRUCTOR

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
		TCONSTRUCTOR()
		Foo() = default;
		TCONSTRUCTOR(Arg=5)
		Foo(int value);

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