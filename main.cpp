#include "boost_folder/boost_test.h"
#include <iostream>
#include <boost/math/distributions/normal.hpp>
// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or
// click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
int main() {
    // TIP Press <shortcut actionId="RenameElement"/> when your caret is at the
    // <b>lang</b> variable name to see how CLion can help you rename it.
    normal_output();
    auto lang = "C++";
    std::cout << "Hello and welcome to " << lang << "!\n";

    boost::math::normal s(1,2); // (default mean = zero, and standard deviation = unity)
    std::cout << "Standard normal distribution, mean = " << s.mean()
        << ", standard deviation = " << s.standard_deviation() << std::endl;

    std::cout << "cdf is " << cdf(s, 0) << std::endl;

    for (int i = 1; i <= 5; i++) {
        // TIP Press <shortcut actionId="Debug"/> to start debugging your code.
        // We have set one <icon src="AllIcons.Debugger.Db_set_breakpoint"/>
        // breakpoint for you, but you can always add more by pressing
        // <shortcut actionId="ToggleLineBreakpoint"/>.
        std::cout << "i = " << i << std::endl;
    }

    return 0;
}

// TIP See CLion help at <a
// href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>.
//  Also, you can try interactive lessons for CLion by selecting
//  'Help | Learn IDE Features' from the main menu.