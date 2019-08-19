# FoodCalc

by [Jesper Lauritsen](https://www.linkedin.com/in/jesperlauritsen/)

FoodCalc is a simple program to calculate intake of nutrients when given a list with amounts of different foods consumed and another list with contents of nutrients for the different foods (a food table).  
> NOTE: FoodCalc was last updated in 2001, and was just moved unchanged to github in 2019 after its previous home at the [University of Copenhagen](www.ibt.ku.dk/jesper/foodcalc/) ceased to exists. I do not plan to make any changes to FoodCalc in the future, but perhaps also none is needed - currently FoodCalc is still in active use at several research institutions around the world, and every year sees a new list of publications with reference to FoodCalc. Feel free to use FoodCalc, but of course make sure to reference it in your publications: "Lauritsen J (2019) Foodcalc v.1.3. https://github.com/jesperldk/FoodCalc (accessed August 2019)"

The calculations done by FoodCalc are rather simple, however it does have some flexibility especially in how to do reductions of nutrients when foods are cooked by boiling, frying, etc.  
FoodCalc is very efficient, making it possibly quite quickly to compute intakes for tens or even hundreds of thousands of people with each hundreds or thousands of foods consumed. The time taken for FoodCalc to do the calculations will largely depend on the computer used. At the time of writing FoodCalc is available for Windows 95/98/NT and for HP-UX. Also the source code is available, and can be easily compiled on most systems that has an ANSI C compiler.

The FoodCalc program was written by [Jesper Lauritsen](http://hjem.get2net.dk/conjes/english/jesper.htm). The writing of the initial version and some of the later changes was largely funded by the [Diet, Cancer and Health project](http://www.cancer.dk/forsk/kb/epi/kost.html) at the [Danish Cancer Society](http://www.cancer.dk/english/).  
The program is in the public domain. You may use it and change it in any way you like. However, you should always give due credit to Jesper Lauritsen. Also, neither Jesper Lauritsen nor the Danish Cancer Society can in no way be responsibly for any damages done by this program. Also, neither Jesper Lauritsen nor the Danish Cancer Society gives any guaranties what so ever regarding the functionality or correctness of this program. Basically you can use it if you want, but if it does not follow your expectations, you have no one to blame but your self. If you have any questions, comments, bug reports, bug fixes, or enhancements, please report it at its github home.

**Why you should _not_ use FoodCalc:**

- If you want to calculate nutrient intakes just for your self and a few friends. FoodCalc is not at all useful for this!
- If you do not have a food table. FoodCalc does _not_ include a food table. However it can be used with the large USDA food table which is freely available, and a subset of the Danish national food table is available for download here.
- If you do not have any experience with command based statistical tools like SAS or SPSS or you do not have the help of a programmer. FoodCalc has no Windows, forms or buttons - you have to write small specialized programs!
- If you do not have a statistical and/or graphical package. FoodCalc can not produce any (non-trivial) statistics nor any graphs.
- If you do not have knowledge about how food calculations are done. FoodCalc has lots of different ways of doing the calculations - you need to be able to choose and specify those you need.

**Why you _should_ use FoodCalc:**

- If you have data for thousands, or tens of thousands, or hundred of thousands of persons. FoodCalc is very fast.
- If you have a food table. FoodCalc can be used with almost any food table.
- If you have your data in a database system, an interview support system or some other system or in text files. It is usually quite easy to export the data to a form readable by FoodCalc.
- If you want full control on how the food calculations are done. FoodCalc can do a lot of different specialized calculations.
- If you want to analyze the resulting data in your favorite statistical and graphical package. The results of FoodCalc calculations can usually very easily be read into most packages.
- If you want to do food calculations as a part of a larger system. FoodCalc can effectively be integrated in many environments and it is highly portable.

**Are you Danish?**

Then you should start by [reading this short introduction to the use of FoodCalc together with the Danish national food table](oversigt.htm). This document is in Danish.

If you are using a variation of the questionnaire developed for the Danish Diet, Cancer and Health project you should definitely contact them for information about conversion to grams per day, for recipes, and for use with FoodCalc.

* * *

      20010118, minor correction of documentation formats in 20190819