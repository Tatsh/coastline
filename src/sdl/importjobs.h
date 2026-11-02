#pragma once

#include <functional>
#include <optional>
#include <span>
#include <string>

#include "importer.h"

namespace coastline {

/**
 * Independent import work in a private destination directory.
 * Report progress from zero to one and stop when the callback returns false.
 * Return an error on failure, or an empty optional on success.
 */
using ImportJob = std::function<std::optional<std::string>(const ImportProgress &)>;

/**
 * Run independent jobs with at most four workers, limited by available CPU concurrency.
 * Progress callbacks are serialised, may run on any worker, and have a monotonic combined fraction.
 * Cancellation and failures stop scheduling work and request active jobs to stop through progress.
 * All workers finish before this function returns, including when a job or callback throws.
 * A rejected progress report cancels its job regardless of the job's returned error text.
 * A conversion failure takes precedence over cancellation of other jobs in the batch.
 *
 * @param jobs Independent jobs with equal progress weights; empty input succeeds.
 * @param progress Combined progress, or empty. Return false to cancel the complete batch.
 * @return An error from a failed job, a cancellation message, or an empty optional on success.
 */
std::optional<std::string> RunImportJobs(std::span<const ImportJob> jobs,
                                         const ImportProgress &progress = {});

} // namespace coastline
