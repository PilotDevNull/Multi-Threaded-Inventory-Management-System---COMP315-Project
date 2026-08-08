#ifndef ANALYTICSAGGREGATOR_H
#define ANALYTICSAGGREGATOR_H

#include "AnalyticsSnapshotDto.h"
#include "LogsSnapshotDto.h"
#include "OrdersSummaryDto.h"

class AnalyticsManager;

namespace AnalyticsAggregator {

AnalyticsSnapshotDto buildAnalytics(const OrdersSummaryDto &summary,
                                    const LogsSnapshotDto &logs);

AnalyticsSnapshotDto buildAnalytics(const OrdersSummaryDto &summary,
                                    const LogsSnapshotDto &logs,
                                    const AnalyticsManager &analyticsManager);

} // namespace AnalyticsAggregator

#endif // ANALYTICSAGGREGATOR_H
