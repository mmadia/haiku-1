#include <TestSuite.h>
#include <TestSuiteAddon.h>

// ##### Include headers for your tests here #####
#include "barchivable/ArchivableTest.h"
#include "bautolock/AutolockTest.h"
#include "blocker/LockerTest.h"
#include "bmemoryio/MemoryIOTest.h"
#include "bmemoryio/MallocIOTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("Support");

	// ##### Add test suites here #####
	suite->addTest("BArchivable", ArchivableTestSuite());
	suite->addTest("BAutolock", AutolockTestSuite());
	suite->addTest("BLocker", LockerTestSuite());
	suite->addTest("BMemoryIO", MemoryIOTestSuite());
	suite->addTest("BMallocIO", MallocIOTestSuite());
	
	return suite;
}
