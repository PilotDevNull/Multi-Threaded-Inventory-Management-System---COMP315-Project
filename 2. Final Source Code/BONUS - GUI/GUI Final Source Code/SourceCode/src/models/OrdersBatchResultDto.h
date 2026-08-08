#ifndef ORDERSBATCHRESULTDTO_H
#define ORDERSBATCHRESULTDTO_H

#include <QString>

#include "AnalyticsSnapshotDto.h"
#include "LogsSnapshotDto.h"
#include "OrdersSummaryDto.h"

struct OrdersBatchResultDto {
    OrdersSummaryDto  summary;
    LogsSnapshotDto   logsSnapshot;   // per-order detail for the Logs page
    AnalyticsSnapshotDto analyticsSnapshot;
    QString           errorMessage;

    bool isValid() const
    {
        return errorMessage.isEmpty();
    }
};

#endif // ORDERSBATCHRESULTDTO_H
