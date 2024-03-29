/*
 * Copyright (C) 2013-2014 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <cppcutter.h>
#include <cutter.h>
#include <gcutter.h>
#include "Hatohol.h"
#include "DBTablesMonitoring.h"
#include "Helpers.h"
#include "DBTablesTest.h"
#include "Params.h"
#include "ThreadLocalDBCache.h"
#include "testDBTablesMonitoring.h"
#include <algorithm>
#include "TestHostResourceQueryOption.h"

using namespace std;
using namespace mlpl;

namespace testDBTablesMonitoring {

#define DECLARE_DBTABLES_MONITORING(VAR_NAME) \
	DBHatohol _dbHatohol; \
	DBTablesMonitoring &VAR_NAME = _dbHatohol.getDBTablesMonitoring();

static const string serverIdColumnName = "server_id";
static const string hostgroupIdColumnName = "host_group_id";
static const string hostIdColumnName = "host_id";

static void addTriggerInfo(TriggerInfo *triggerInfo)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	dbMonitoring.addTriggerInfo(triggerInfo);
}
#define assertAddTriggerToDB(X) \
cut_trace(_assertAddToDB<TriggerInfo>(X, addTriggerInfo))

struct AssertGetTriggersArg
  : public AssertGetHostResourceArg<TriggerInfo, TriggersQueryOption>
{
	AssertGetTriggersArg(gconstpointer ddtParam)
	{
		fixtures = testTriggerInfo;
		numberOfFixtures = NumTestTriggerInfo;
		setDataDrivenTestParam(ddtParam);
	}

	virtual LocalHostIdType getHostId(const TriggerInfo &info) const override
	{
		return info.hostIdInServer;
	}

	virtual string makeOutputText(const TriggerInfo &triggerInfo)
	{
		return makeTriggerOutput(triggerInfo);
	}
};

static void _assertGetTriggers(AssertGetTriggersArg &arg)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	arg.fixup();
	dbMonitoring.getTriggerInfoList(arg.actualRecordList, arg.option);
	arg.assert();
}
#define assertGetTriggers(A) cut_trace(_assertGetTriggers(A))

static void _assertGetTriggersWithFilter(AssertGetTriggersArg &arg)
{
	// setup trigger data
	void test_addTriggerInfoList(gconstpointer data);
	test_addTriggerInfoList(arg.ddtParam);
	assertGetTriggers(arg);
}
#define assertGetTriggersWithFilter(ARG) \
cut_trace(_assertGetTriggersWithFilter(ARG))

static void _assertGetTriggerInfoList(
  gconstpointer ddtParam,
  const ServerIdType &serverId,
  const HostIdType &hostId = ALL_HOSTS)
{
	loadTestDBTriggers();
	loadTestDBServerHostDef();
	loadTestDBHostgroupMember();

	AssertGetTriggersArg arg(ddtParam);
	arg.targetServerId = serverId;
	arg.targetHostId = hostId;
	assertGetTriggers(arg);
}
#define assertGetTriggerInfoList(DDT_PARAM, SERVER_ID, ...) \
cut_trace(_assertGetTriggerInfoList(DDT_PARAM, SERVER_ID, ##__VA_ARGS__))

static void _assertGetEvents(AssertGetEventsArg &arg)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	arg.fixup();
	IncidentInfoVect *incidentInfoVectPointer
	  = arg.withIncidentInfo ? &arg.actualIncidentInfoVect : NULL;
	assertHatoholError(
	  arg.expectedErrorCode,
	  dbMonitoring.getEventInfoList(arg.actualRecordList, arg.option,
				     incidentInfoVectPointer));
	if (arg.expectedErrorCode != HTERR_OK)
		return;
	arg.assert();
}
#define assertGetEvents(A) cut_trace(_assertGetEvents(A))

static void _assertGetEventsWithFilter(AssertGetEventsArg &arg)
{
	// setup event data
	void test_addEventInfoList(gconstpointer data);
	test_addEventInfoList(arg.ddtParam);
	if (arg.withIncidentInfo)
		loadTestDBIncidents();

	assertGetEvents(arg);
}
#define assertGetEventsWithFilter(ARG) \
cut_trace(_assertGetEventsWithFilter(ARG))

struct AssertGetItemsArg
  : public AssertGetHostResourceArg<ItemInfo, ItemsQueryOption>
{
	string itemGroupName;

	AssertGetItemsArg(gconstpointer ddtParam)
	{
		fixtures = testItemInfo;
		numberOfFixtures = NumTestItemInfo;
		setDataDrivenTestParam(ddtParam);
	}

	virtual void fixupOption(void) override
	{
		AssertGetHostResourceArg<ItemInfo, ItemsQueryOption>::
			fixupOption();
		option.setTargetItemGroupName(itemGroupName);
	}

	virtual bool filterOutExpectedRecord(const ItemInfo *info) override
	{
		if (AssertGetHostResourceArg<ItemInfo, ItemsQueryOption>
		      ::filterOutExpectedRecord(info)) {
			return true;
		}

		if (!itemGroupName.empty() &&
		    info->itemGroupName != itemGroupName) {
			return true;
		}

		if (filterForDataOfDefunctSv) {
			if (!option.isValidServer(info->serverId))
				return true;
		}

		return false;
	}

	virtual LocalHostIdType getHostId(const ItemInfo &info) const override
	{
		return info.hostIdInServer;
	}

	virtual string makeOutputText(const ItemInfo &itemInfo) override
	{
		return makeItemOutput(itemInfo);
	}
};

static void _assertGetItems(AssertGetItemsArg &arg)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	arg.fixup();
	dbMonitoring.getItemInfoList(arg.actualRecordList, arg.option);
	arg.assert();
}
#define assertGetItems(A) cut_trace(_assertGetItems(A))

static void _assertGetItemsWithFilter(AssertGetItemsArg &arg)
{
	// setup item data
	void test_addItemInfoList(gconstpointer data);
	test_addItemInfoList(arg.ddtParam);
	assertGetItems(arg);
}
#define assertGetItemsWithFilter(ARG) \
cut_trace(_assertGetItemsWithFilter(ARG))

static void _assertGetNumberOfItems(AssertGetItemsArg &arg)
{
	void test_addItemInfoList(gconstpointer data);
	test_addItemInfoList(arg.ddtParam);
	arg.fixup();
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	cppcut_assert_equal(arg.expectedRecords.size(),
			    dbMonitoring.getNumberOfItems(arg.option));
}
#define assertGetNumberOfItems(A) cut_trace(_assertGetNumberOfItems(A))

void _assertItemInfoList(gconstpointer data, uint32_t serverId)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	ItemInfoList itemInfoList;
	for (size_t i = 0; i < NumTestItemInfo; i++)
		itemInfoList.push_back(testItemInfo[i]);
	dbMonitoring.addItemInfoList(itemInfoList);

	AssertGetItemsArg arg(data);
	arg.targetServerId = serverId;
	assertGetItems(arg);
}
#define assertItemInfoList(DATA, SERVER_ID) \
cut_trace(_assertItemInfoList(DATA, SERVER_ID))

static void _assertGetNumberOfHostsWithUserAndStatus(UserIdType userId, bool status)
{
	loadTestDBTriggers();

	const ServerIdType serverId = testTriggerInfo[0].serverId;
	// TODO: should give the appropriate host group ID after
	//       Hatohol support it.
	const HostgroupIdType hostgroupId = ALL_HOST_GROUPS;

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	int expected = getNumberOfTestHostsWithStatus(serverId, hostgroupId,
	                                              status, userId);
	int actual;
	TriggersQueryOption option(userId);
	option.setTargetServerId(serverId);
	option.setTargetHostgroupId(hostgroupId);

	if (status)
		actual  = dbMonitoring.getNumberOfGoodHosts(option);
	else
		actual  = dbMonitoring.getNumberOfBadHosts(option);
	cppcut_assert_equal(expected, actual);
}
#define assertGetNumberOfHostsWithUserAndStatus(U, ST) \
cut_trace(_assertGetNumberOfHostsWithUserAndStatus(U, ST))

static
void _assertTriggerInfo(const TriggerInfo &expect, const TriggerInfo &actual)
{
	cppcut_assert_equal(expect.serverId, actual.serverId);
	cppcut_assert_equal(expect.id, actual.id);
	cppcut_assert_equal(expect.status, actual.status);
	cppcut_assert_equal(expect.severity, actual.severity);
	cppcut_assert_equal(expect.lastChangeTime.tv_sec,
	                    actual.lastChangeTime.tv_sec);
	cppcut_assert_equal(expect.lastChangeTime.tv_nsec,
	                    actual.lastChangeTime.tv_nsec);
	cppcut_assert_equal(expect.globalHostId, actual.globalHostId);
	cppcut_assert_equal(expect.hostIdInServer, actual.hostIdInServer);
	cppcut_assert_equal(expect.hostName, actual.hostName);
	cppcut_assert_equal(expect.brief, actual.brief);
}
#define assertTriggerInfo(E,A) cut_trace(_assertTriggerInfo(E,A))

static void prepareDataForAllHostgroupIds(void)
{
	set<HostgroupIdType> hostgroupIdSet = getTestHostgroupIdSet();
	hostgroupIdSet.insert(ALL_HOST_GROUPS);
	set<HostgroupIdType>::const_iterator hostgrpIdItr =
	  hostgroupIdSet.begin();
	for (; hostgrpIdItr != hostgroupIdSet.end(); ++hostgrpIdItr) {
		string label = "Hostgroup ID: ";
		label += *hostgrpIdItr;
		gcut_add_datum(label.c_str(), "hostgroupId",
		               G_TYPE_STRING, hostgrpIdItr->c_str(), NULL);
	}
}

void cut_setup(void)
{
	hatoholInit();
	setupTestDB();
	loadTestDBTablesConfig();
	loadTestDBTablesUser();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_tablesVersion(void)
{
	// create an instance
	// Tables in the DB will be automatically created.
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	assertDBTablesVersion(
	  dbMonitoring.getDBAgent(),
	  DB_TABLES_ID_MONITORING, DBTablesMonitoring::MONITORING_DB_VERSION);
}


void test_createTableTrigger(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	// check content
	string statement = "SELECT * FROM triggers";
	string expectedOut = ""; // currently no data
	assertDBContent(&dbMonitoring.getDBAgent(), statement, expectedOut);
}

void test_addTriggerInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	// added a record
	TriggerInfo *testInfo = testTriggerInfo;
	assertAddTriggerToDB(testInfo);

	// confirm with the command line tool
	string sql = StringUtils::sprintf("SELECT * from triggers");
	string expectedOut = makeTriggerOutput(*testInfo);
	assertDBContent(&dbMonitoring.getDBAgent(), sql, expectedOut);
}

void test_deleteTriggerInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBTriggers();

	TriggerIdList triggerIdList = { "2", "3" };
	constexpr ServerIdType targetServerId = 1;

	for (auto triggerId : triggerIdList) {
		// check triggerInfo existence
		string sql = StringUtils::sprintf("SELECT * FROM triggers");
		sql += StringUtils::sprintf(
		  " WHERE id = '%" FMT_TRIGGER_ID "'"
		  " AND server_id = %" FMT_SERVER_ID,
		  triggerId.c_str(), targetServerId);
		uint64_t targetTestDataId = StringUtils::toUint64(triggerId) - 1;
		string expectedOut;
		expectedOut +=
			makeTriggerOutput(testTriggerInfo[targetTestDataId]);
		assertDBContent(&dbMonitoring.getDBAgent(), sql, expectedOut);
	}

	HatoholError err =
		dbMonitoring.deleteTriggerInfo(triggerIdList, targetServerId);
	assertHatoholError(HTERR_OK, err);
	for (auto triggerId : triggerIdList) {
		string statement = StringUtils::sprintf("SELECT * FROM triggers");
		statement += StringUtils::sprintf(
		  " WHERE id = %" FMT_TRIGGER_ID " AND server_id = %" FMT_SERVER_ID,
		  triggerId.c_str(), targetServerId);
		string expected = "";
		assertDBContent(&dbMonitoring.getDBAgent(), statement, expected);
	}
}

void test_syncTriggers(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBTriggers();
	constexpr const ServerIdType targetServerId = 1;
	const TriggerIdType targetTriggerId = "2";
	constexpr const int testTriggerDataId = 2;
	const TriggerIdList targetServerTriggerIds = {"1", "2", "3", "4", "5"};

	string sql = StringUtils::sprintf("SELECT * FROM triggers");
	sql += StringUtils::sprintf(
	  " WHERE server_id = %" FMT_SERVER_ID " ORDER BY server_id ASC",
	  targetServerId);

	string expectedOut;
	// check triggerInfo existence
	for (auto triggerId : targetServerTriggerIds) {
		int64_t targetId = StringUtils::toUint64(triggerId) - 1;
		expectedOut += makeTriggerOutput(testTriggerInfo[targetId]);
	}
	assertDBContent(&dbMonitoring.getDBAgent(), sql, expectedOut);

	map<TriggerIdType, const TriggerInfo *> triggerMap;
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &svTriggerInfo = testTriggerInfo[i];
		if (svTriggerInfo.serverId != targetServerId)
			continue;
		if (svTriggerInfo.id != targetTriggerId)
			continue;
		triggerMap[svTriggerInfo.id] = &svTriggerInfo;
	}

	TriggerInfoList svTriggers =
	{
		{
			testTriggerInfo[testTriggerDataId - 1]
		}
	};

	// sanity check if we use the proper data
	cppcut_assert_equal(false, svTriggers.empty());
	// Prepare for the expected result.
	string expect;
	for (auto triggerPair : triggerMap) {
		const TriggerInfo svTrigger = *triggerPair.second;
		expect += StringUtils::sprintf(
		  "%" FMT_LOCAL_HOST_ID "|%s\n",
		  svTrigger.hostIdInServer.c_str(), svTrigger.hostName.c_str());
	}
	HatoholError err = dbMonitoring.syncTriggers(svTriggers, targetServerId);
	assertHatoholError(HTERR_OK, err);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = StringUtils::sprintf(
	  "select host_id_in_server,hostname from triggers"
	  " where server_id=%" FMT_SERVER_ID " order by id asc;",
	  targetServerId);
	assertDBContent(&dbAgent, statement, expect);
}

void test_syncTriggersAddNewTrigger(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBTriggers();
	constexpr const ServerIdType targetServerId = 1;

	TriggerInfo newTriggerInfo = {
		1,                        // serverId
		"7",                      // id
		TRIGGER_STATUS_OK,        // status
		TRIGGER_SEVERITY_INFO,    // severity
		{1362958197,0},           // lastChangeTime
		10,                       // globalHostId,
		"235013",                 // hostIdInServer,
		"hostX2",                 // hostName,
		"TEST New Trigger 1",     // brief,
		"",                       // extendedInfo
		TRIGGER_VALID,            // validity
	};

	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &svTriggerInfo = testTriggerInfo[i];
		if (svTriggerInfo.serverId != targetServerId)
			continue;
		if (svTriggerInfo.id == newTriggerInfo.id)
			cut_fail("We use the wrong test data");
	}

	string expect;
	TriggerInfoList svTriggers;
	{
		size_t i = 0;
		for (; i < NumTestTriggerInfo; i++) {
			const TriggerInfo &svTriggerInfo = testTriggerInfo[i];
			if (svTriggerInfo.serverId != targetServerId)
				continue;
			svTriggers.push_back(svTriggerInfo);
			expect += makeTriggerOutput(svTriggerInfo);
		}
		// sanity check if we use the proper data
		cppcut_assert_equal(false, svTriggers.empty());

		// Add newTriggerInfo to the expected result
		svTriggers.push_back(newTriggerInfo);
		expect += makeTriggerOutput(newTriggerInfo);
	}
	HatoholError err = dbMonitoring.syncTriggers(svTriggers, targetServerId);
	assertHatoholError(HTERR_OK, err);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = StringUtils::sprintf(
	  "select * from triggers"
	  " where server_id=%" FMT_SERVER_ID " order by id asc;",
	  targetServerId);
	assertDBContent(&dbAgent, statement, expect);
}

void test_syncTriggersModifiedTrigger(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBTriggers();
	constexpr const ServerIdType targetServerId = 1;

	TriggerInfo modifiedTriggerInfo = {
		1,                        // serverId
		"1",                      // id
		TRIGGER_STATUS_OK,        // status
		TRIGGER_SEVERITY_INFO,    // severity
		{1362957197,0},           // lastChangeTime
		10,                       // globalHostId,
		"235012",                 // hostIdInServer,
		"hostX1 revised",         // hostName,
		"TEST Trigger 1 Revised", // brief,
		"{\"expandedDescription\":\"Test Trigger on hostX1 revised\"}", // extendedInfo
		TRIGGER_VALID,            // validity
	};

	// sanity check for test data
	for (size_t i = 0; i < NumTestTriggerInfo; i++) {
		const TriggerInfo &svTriggerInfo = testTriggerInfo[i];
		if (svTriggerInfo.serverId != targetServerId)
			continue;
		if (svTriggerInfo.brief == modifiedTriggerInfo.brief)
			cut_fail("We use the wrong test data");
	}

	string expect;
	TriggerInfoList svTriggers;
	{
		size_t i = 0;
		// Add modifiedTriggerInfo to the expected result
		svTriggers.push_back(modifiedTriggerInfo);
		expect += makeTriggerOutput(modifiedTriggerInfo);

		for (i = 1; i < NumTestTriggerInfo; i++) {
			const TriggerInfo &svTriggerInfo = testTriggerInfo[i];
			if (svTriggerInfo.serverId != targetServerId)
				continue;
			svTriggers.push_back(svTriggerInfo);
			expect += makeTriggerOutput(svTriggerInfo);
		}
		// sanity check if we use the proper data
		cppcut_assert_equal(false, svTriggers.empty());
	}
	HatoholError err = dbMonitoring.syncTriggers(svTriggers, targetServerId);
	assertHatoholError(HTERR_OK, err);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = StringUtils::sprintf(
	  "select * from triggers"
	  " where server_id=%" FMT_SERVER_ID " order by id asc;",
	  targetServerId);
	assertDBContent(&dbAgent, statement, expect);
}

void test_getTriggerInfo(void)
{
	loadTestDBTriggers();
	loadTestDBServerHostDef();
	loadTestDBHostgroupMember();

	int targetIdx = 2;
	TriggerInfo &targetTriggerInfo = testTriggerInfo[targetIdx];
	TriggerInfo triggerInfo;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(targetTriggerInfo.serverId);
	option.setTargetId(targetTriggerInfo.id);
	cppcut_assert_equal(true,
	   dbMonitoring.getTriggerInfo(triggerInfo, option));
	assertTriggerInfo(targetTriggerInfo, triggerInfo);
}

void test_getTriggerInfoNotFound(void)
{
	loadTestDBTriggers();

	const UserIdType invalidSvId = -1;
	const TriggerIdType invalidTrigId;
	TriggerInfo triggerInfo;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(invalidSvId);
	option.setTargetId(invalidTrigId);
	cppcut_assert_equal(false,
	                    dbMonitoring.getTriggerInfo(triggerInfo, option));
}

void data_getTriggerInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerInfoList(gconstpointer data)
{
	assertGetTriggerInfoList(data, ALL_SERVERS);
}

void data_getTriggerInfoListForOneServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerInfoListForOneServer(gconstpointer data)
{
	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	assertGetTriggerInfoList(data, targetServerId);
}

void data_getTriggerInfoListForOneServerOneHost(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerInfoListForOneServerOneHost(gconstpointer data)
{
	const ServerIdType targetServerId = testTriggerInfo[1].serverId;
	const HostIdType targetHostId = testTriggerInfo[1].globalHostId;
	assertGetTriggerInfoList(data, targetServerId, targetHostId);
}

void data_setTriggerInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_setTriggerInfoList(gconstpointer data)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggerInfoList triggerInfoList;
	for (size_t i = 0; i < NumTestTriggerInfo; i++)
		triggerInfoList.push_back(testTriggerInfo[i]);
	const ServerIdType serverId = testTriggerInfo[0].serverId;
	dbMonitoring.setTriggerInfoList(triggerInfoList, serverId);

	AssertGetTriggersArg arg(data);
	assertGetTriggers(arg);
}

void data_addTriggerInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_addTriggerInfoList(gconstpointer data)
{
	size_t i;
	DECLARE_DBTABLES_MONITORING(dbMonitoring);

	// First call
	size_t numFirstAdd = NumTestTriggerInfo / 2;
	TriggerInfoList triggerInfoList0;
	for (i = 0; i < numFirstAdd; i++)
		triggerInfoList0.push_back(testTriggerInfo[i]);
	dbMonitoring.addTriggerInfoList(triggerInfoList0);

	// Second call
	TriggerInfoList triggerInfoList1;
	for (; i < NumTestTriggerInfo; i++)
		triggerInfoList1.push_back(testTriggerInfo[i]);
	dbMonitoring.addTriggerInfoList(triggerInfoList1);

	// Check
	AssertGetTriggersArg arg(data);
	assertGetTriggers(arg);
}

void data_getTriggerWithOneAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetTriggersArg arg(data);
	arg.userId = 5;
	assertGetTriggersWithFilter(arg);
}

void data_getTriggerWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetTriggersArg arg(data);
	arg.userId = 4;
	assertGetTriggersWithFilter(arg);
}

void data_getTriggerWithInvalidUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getTriggerWithInvalidUserId(gconstpointer data)
{
	AssertGetTriggersArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetTriggersWithFilter(arg);
}

void data_itemInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_itemInfoList(gconstpointer data)
{
	assertItemInfoList(data, ALL_SERVERS);
}

void data_itemInfoListForOneServer(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_itemInfoListForOneServer(gconstpointer data)
{
	const ServerIdType targetServerId = testItemInfo[0].serverId;
	assertItemInfoList(data, targetServerId);
}

void data_addItemInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_addItemInfoList(gconstpointer data)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	ItemInfoList itemInfoList;
	for (size_t i = 0; i < NumTestItemInfo; i++)
		itemInfoList.push_back(testItemInfo[i]);
	dbMonitoring.addItemInfoList(itemInfoList);

	AssertGetItemsArg arg(data);
	assertGetItems(arg);
}

void data_getItemsWithOneAuthorizedServer(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getItemsWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 5;
	assertGetItemsWithFilter(arg);
}

void data_getItemWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getItemWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 4;
	assertGetItemsWithFilter(arg);
}

void data_getItemWithInvalidUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getItemWithInvalidUserId(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetItemsWithFilter(arg);
}

void data_getItemWithItemGroupName(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getItemWithItemGroupName(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.itemGroupName = "City";
	assertGetItemsWithFilter(arg);
}

void data_getNumberOfItemsWithOneAuthorizedServer(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getNumberOfItemsWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 5;
	assertGetNumberOfItems(arg);
}

void data_getNumberOfItemWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getNumberOfItemWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = 4;
	assertGetNumberOfItems(arg);
}

void data_getNumberOfItemWithInvalidUserId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getNumberOfItemWithInvalidUserId(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetNumberOfItems(arg);
}

void data_getNumberOfItemsWithItemGroupName(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getNumberOfItemsWithItemGroupName(gconstpointer data)
{
	AssertGetItemsArg arg(data);
	arg.itemGroupName = "City";
	assertGetNumberOfItems(arg);
}

void data_addEventInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_addEventInfoList(gconstpointer data)
{
	// DBTablesMonitoring internally joins the trigger table and the event table.
	// So we also have to add trigger data.
	// When the internal join is removed, the following line will not be
	// needed.
	test_setTriggerInfoList(data);

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventInfoList eventInfoList;
	for (size_t i = 0; i < NumTestEventInfo; i++)
		eventInfoList.push_back(testEventInfo[i]);
	dbMonitoring.addEventInfoList(eventInfoList);

	AssertGetEventsArg arg(data);
	assertGetEvents(arg);
}

void test_addEventInfoUnifiedId(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventInfoList eventInfoList;
	for (size_t i = 0; i < NumTestEventInfo; i++)
		eventInfoList.push_back(testEventInfo[i]);
	dbMonitoring.addEventInfoList(eventInfoList);

	EventInfoListIterator it = eventInfoList.begin();
	string expected;
	string actual;
	for (int i = 1; it != eventInfoList.end(); it++, i++) {
		if (!expected.empty())
			expected += "\n";
		if (!actual.empty())
			actual += "\n";
		expected += StringUtils::toString(i);
		expected += string("|") + makeEventOutput(*it);
		actual += StringUtils::toString(it->unifiedId);
		actual += string("|") + makeEventOutput(*it);
	}
	cppcut_assert_equal(expected, actual);
	assertDBContent(&dbMonitoring.getDBAgent(),
			"select * from events", expected);
}

void data_addDupEventInfoList(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_addDupEventInfoList(gconstpointer data)
{
	test_setTriggerInfoList(data);

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	EventInfoList eventInfoList;
	for (size_t i = 0; i < NumTestDupEventInfo; i++){
		eventInfoList.push_back(testDupEventInfo[i]);
	}
	dbMonitoring.addEventInfoList(eventInfoList);

	AssertGetEventsArg arg(data, testDupEventInfo, NumTestDupEventInfo);
	assertGetEvents(arg);
}

void data_getLastEventId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getMaxEventId(gconstpointer data)
{
	test_addEventInfoList(data);
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverid = 3;
	cppcut_assert_equal(findLastEventId(serverid),
	                    dbMonitoring.getMaxEventId(serverid));
}

void test_getMaxEventIdWithNoEvent(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverid = 3;
	cppcut_assert_equal(EVENT_NOT_FOUND,
	                    dbMonitoring.getMaxEventId(serverid));
}

void test_getTimeOfLastEvent(void)
{
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverId = 1;
	cppcut_assert_equal(findTimeOfLastEvent(serverId),
	                    dbMonitoring.getTimeOfLastEvent(serverId));
}

void test_getTimeOfLastEventWithTriggerId(void)
{
	loadTestDBEvents();

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverId = 3;
	const TriggerIdType triggerId = "3";
	cppcut_assert_equal(
	  findTimeOfLastEvent(serverId, triggerId),
	  dbMonitoring.getTimeOfLastEvent(serverId, triggerId));
}

void data_getNumberOfTriggers(void)
{
	prepareDataForAllHostgroupIds();
}

void test_getNumberOfTriggers(gconstpointer data)
{
	loadTestDBTriggers();
	loadTestDBHostgroupMember();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId =
	  gcut_data_get_string(data, "hostgroupId");

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(targetServerId);
	option.setTargetHostgroupId(hostgroupId);
	cppcut_assert_equal(
	  getNumberOfTestTriggers(targetServerId, hostgroupId),
	  dbMonitoring.getNumberOfTriggers(option),
	  cut_message("sv: %" FMT_SERVER_ID ", hostgroup: %" FMT_HOST_GROUP_ID,
	              targetServerId, hostgroupId.c_str()));
}

void test_getNumberOfTriggersForMultipleAuthorizedHostgroups(void)
{
	loadTestDBTriggers();
	loadTestDBHostgroupMember();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId = ALL_HOST_GROUPS;

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	TriggersQueryOption option(userIdWithMultipleAuthorizedHostgroups);
	option.setTargetServerId(targetServerId);
	option.setTargetHostgroupId(hostgroupId);

	cppcut_assert_equal(
	  getNumberOfTestTriggers(targetServerId, hostgroupId),
	  dbMonitoring.getNumberOfTriggers(option));
}

void data_getNumberOfTriggersBySeverity(void)
{
	prepareDataForAllHostgroupIds();
}

void _assertGetNumberOfTriggers(DBTablesMonitoring &dbMonitoring,
				const ServerIdType &serverId,
				const HostgroupIdType &hostgroupId,
				const TriggerSeverityType &severity)
{
	TriggersQueryOption option(USER_ID_SYSTEM);
	option.setTargetServerId(serverId);
	option.setTargetHostgroupId(hostgroupId);
	const size_t actual =
	  dbMonitoring.getNumberOfBadTriggers(option, severity);
	const size_t expect = getNumberOfTestTriggers(
	  serverId, hostgroupId, severity);
	if (isVerboseMode())
		cut_notify("The expected number of triggesr: %zd\n", expect);
	cppcut_assert_equal(expect, actual,
			    cut_message(
			      "sv: %" FMT_SERVER_ID ", "
			      "hostgroup: %" FMT_HOST_GROUP_ID ", "
			      "severity: %d",
			      serverId, hostgroupId.c_str(), severity));
}
#define assertGetNumberOfTriggers(D,S,H,V) \
cut_trace(_assertGetNumberOfTriggers(D,S,H,V))

void test_getNumberOfTriggersBySeverity(gconstpointer data)
{
	loadTestDBTriggers();
	loadTestDBHostgroupMember();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId =
	  gcut_data_get_string(data, "hostgroupId");

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	for (int i = 0; i < NUM_TRIGGER_SEVERITY; i++) {
		const TriggerSeverityType severity
		  = static_cast<TriggerSeverityType>(i);
		assertGetNumberOfTriggers(
		  dbMonitoring, targetServerId, hostgroupId, severity);
	}
}

void data_getNumberOfAllBadTriggers(void)
{
	prepareDataForAllHostgroupIds();
}

void test_getNumberOfAllBadTriggers(gconstpointer data)
{
	loadTestDBTriggers();
	loadTestDBHostgroupMember();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	const HostgroupIdType hostgroupId =
	  gcut_data_get_string(data, "hostgroupId");

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	assertGetNumberOfTriggers(dbMonitoring, targetServerId, hostgroupId,
				  TRIGGER_SEVERITY_ALL);
}

void test_getNumberOfTriggersBySeverityWithoutPriviledge(void)
{
	loadTestDBTriggers();

	const ServerIdType targetServerId = testTriggerInfo[0].serverId;
	// TODO: should give the appropriate host group ID after
	//       Hatohol support it.
	//uint64_t hostgroupId = 0;

	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	for (int i = 0; i < NUM_TRIGGER_SEVERITY; i++) {
		TriggersQueryOption option;
		option.setTargetServerId(targetServerId);
		//TODO: uncomment it after Hatohol supports host group
		//option.setTargetHostgroupId(hostgroupId);
		TriggerSeverityType severity = (TriggerSeverityType)i;
		size_t actual
		  = dbMonitoring.getNumberOfBadTriggers(option, severity);
		size_t expected = 0;
		cppcut_assert_equal(expected, actual,
		                    cut_message("severity: %d", i));
	}
}

void test_getLastChangeTimeOfTriggerWithNoTrigger(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const ServerIdType serverid = 3;
	cppcut_assert_equal(
	  0, dbMonitoring.getLastChangeTimeOfTrigger(serverid));
}

void test_getNumberOfGoodHosts(void)
{
	assertGetNumberOfHostsWithUserAndStatus(USER_ID_SYSTEM, true);
}

void test_getNumberOfBadHosts(void)
{
	assertGetNumberOfHostsWithUserAndStatus(USER_ID_SYSTEM, false);
}

void test_getNumberOfGoodHostsWithNoAuthorizedServer(void)
{
	UserIdType userId = 4;
	assertGetNumberOfHostsWithUserAndStatus(userId, true);
}

void test_getNumberOfBadHostsWithNoAuthorizedServer(void)
{
	UserIdType userId = 4;
	assertGetNumberOfHostsWithUserAndStatus(userId, false);
}

void test_getNumberOfGoodHostsWithOneAuthorizedServer(void)
{
	UserIdType userId = 5;
	assertGetNumberOfHostsWithUserAndStatus(userId, true);
}

void test_getNumberOfBadHostsWithOneAuthorizedServer(void)
{
	UserIdType userId = 5;
	assertGetNumberOfHostsWithUserAndStatus(userId, false);
}

void test_getEventSortAscending(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventSortAscending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.sortDirection = DataQueryOption::SORT_ASCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventSortDescending(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventSortDescending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.sortDirection = DataQueryOption::SORT_DESCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaximumNumber(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithMaximumNumber(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.maxNumber = 2;
	// In this test, we only specify the maximum number. So the output
	// rows cannot be expected.
	arg.assertContent = false;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaximumNumberDescending(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithMaximumNumberDescending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.maxNumber = 2;
	arg.sortDirection = DataQueryOption::SORT_DESCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaximumNumberAndOffsetAscending(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithMaximumNumberAndOffsetAscending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.maxNumber = 2;
	arg.offset = 1;
	arg.sortDirection = DataQueryOption::SORT_ASCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaximumNumberAndOffsetDescending(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithMaximumNumberAndOffsetDescending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.maxNumber = 2;
	arg.offset = 1;
	arg.sortDirection = DataQueryOption::SORT_DESCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithLimitOfUnifiedIdAscending(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithLimitOfUnifiedIdAscending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.limitOfUnifiedId = 2;
	arg.sortDirection = DataQueryOption::SORT_ASCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithSortTimeAscending(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithSortTimeAscending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.sortType = EventsQueryOption::SORT_TIME;
	arg.sortDirection = DataQueryOption::SORT_ASCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithSortTimeDescending(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithSortTimeDescending(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.sortType = EventsQueryOption::SORT_TIME;
	arg.sortDirection = DataQueryOption::SORT_DESCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithOffsetWithoutLimit(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithOffsetWithoutLimit(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.offset = 2;
	arg.expectedErrorCode = HTERR_OFFSET_WITHOUT_LIMIT;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMaxNumAndOffsetAndLimitOfUnifiedIdDescending(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithMaxNumAndOffsetAndLimitOfUnifiedIdDescending(
  gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.maxNumber = 2;
	arg.offset = 1;
	arg.limitOfUnifiedId = 2;
	arg.sortDirection = DataQueryOption::SORT_DESCENDING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithOneAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithOneAuthorizedServer(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.userId = 5;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithNoAuthorizedServer(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithNoAuthorizedServer(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.userId = 4;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithInvalidUserId(gconstpointer data)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithInvalidUserId(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.userId = INVALID_USER_ID;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithMinSeverity(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithMinSeverity(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.minSeverity = TRIGGER_SEVERITY_WARNING;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithTriggerStatus(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithTriggerStatus(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.triggerStatus = TRIGGER_STATUS_PROBLEM;
	assertGetEventsWithFilter(arg);
}

void data_getEventsWithIncidentInfo(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventsWithIncidentInfo(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.withIncidentInfo = true;
	assertGetEventsWithFilter(arg);
}

void data_getEventsWithIncidentInfoByAuthorizedUser(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventsWithIncidentInfoByAuthorizedUser(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.userId = 5;
	arg.withIncidentInfo = true;
	assertGetEventsWithFilter(arg);
}

void data_getEventWithTriggerId(void)
{
	prepareTestDataForFilterForDataOfDefunctServers();
}

void test_getEventWithTriggerId(gconstpointer data)
{
	AssertGetEventsArg arg(data);
	arg.triggerId = 3;
	assertGetEventsWithFilter(arg);
}

void test_addIncidentInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();
	string statement = "select * from incidents;";
	string expect;

	for (size_t i = 0; i < NumTestIncidentInfo; i++) {
		IncidentInfo expectedIncidentInfo = testIncidentInfo[i];
		expect += makeIncidentOutput(expectedIncidentInfo);
		dbMonitoring.addIncidentInfo(&testIncidentInfo[i]);
	}
	assertDBContent(&dbAgent, statement, expect);
}

void test_updateIncidentInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();

	IncidentInfo incidentInfo = testIncidentInfo[0];
	dbMonitoring.addIncidentInfo(&incidentInfo);
	incidentInfo.status = "Assigned";
	incidentInfo.assignee = "hikeshi";
	incidentInfo.updatedAt.tv_sec = time(NULL);
	incidentInfo.updatedAt.tv_nsec = 0;
	dbMonitoring.addIncidentInfo(&incidentInfo);

	string statement("select * from incidents;");
	string expect(makeIncidentOutput(incidentInfo));
	assertDBContent(&dbAgent, statement, expect);
}

void test_updateIncidentInfoByDedicatedFuction(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	DBAgent &dbAgent = dbMonitoring.getDBAgent();

	IncidentInfo incidentInfo = testIncidentInfo[0];
	dbMonitoring.addIncidentInfo(&incidentInfo);
	incidentInfo.status = "Assigned";
	incidentInfo.assignee = "hikeshi";
	incidentInfo.updatedAt.tv_sec = time(NULL);
	incidentInfo.updatedAt.tv_nsec = 0;
	dbMonitoring.updateIncidentInfo(incidentInfo);

	string statement("select * from incidents;");
	string expect(makeIncidentOutput(incidentInfo));
	assertDBContent(&dbAgent, statement, expect);
}

void test_getIncidentInfo(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	IncidentInfoVect incidents;
	IncidentsQueryOption option(USER_ID_SYSTEM);
	string expected, actual;

	loadTestDBIncidents();
	for (size_t i = 0; i < NumTestIncidentInfo; i++) {
		IncidentInfo expectedIncidentInfo = testIncidentInfo[i];
		expected += makeIncidentOutput(expectedIncidentInfo);
	}

	dbMonitoring.getIncidentInfoVect(incidents, option);
	IncidentInfoVectIterator it = incidents.begin();
	for (; it != incidents.end(); ++it) {
		IncidentInfo &actualIncidentInfo = *it;
		actual += makeIncidentOutput(actualIncidentInfo);
	}

	cppcut_assert_equal(expected, actual);
}

void test_getLastUpdateTimeOfIncidents(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	loadTestDBIncidents();
	const IncidentTrackerIdType trackerId = 3;

	uint64_t expected = 0;
	for (size_t i = 0; i < NumTestIncidentInfo; i++) {
		if (testIncidentInfo[i].trackerId == trackerId &&
		    testIncidentInfo[i].updatedAt.tv_sec > expected) {
			expected = testIncidentInfo[i].updatedAt.tv_sec;
		}
	}

	cppcut_assert_equal(
	  expected, dbMonitoring.getLastUpdateTimeOfIncidents(trackerId));
}

void test_getLastUpdateTimeOfIncidentsWithNoRecord(void)
{
	DECLARE_DBTABLES_MONITORING(dbMonitoring);
	const IncidentTrackerIdType trackerId = 3;
	uint64_t expected = 0;
	cppcut_assert_equal(
	  expected, dbMonitoring.getLastUpdateTimeOfIncidents(trackerId));
}

} // namespace testDBTablesMonitoring
