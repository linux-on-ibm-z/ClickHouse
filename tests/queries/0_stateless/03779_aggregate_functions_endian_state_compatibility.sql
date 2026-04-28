-- Ensure aggregation states are compatible between big and little endian systems.

SET allow_experimental_funnel_functions = 1;

select 'sum/sumWithOverflow/sumKahan';
select hex(sumState(15::UInt64));
select hex(sumWithOverflowState(15::UInt64));
select hex(sumKahanState(15.5::Float64));

select 'groupArraySorted';
select hex(groupArraySortedState(10)(number)) from numbers(5);

select 'contingency/cramersV/cramersVBiasCorrected/theilsU';
select hex(contingencyState(number % 3, number % 2)) from numbers(10);
select hex(cramersVState(number % 3, number % 2)) from numbers(10);
select hex(cramersVBiasCorrectedState(number % 3, number % 2)) from numbers(10);
select hex(theilsUState(number % 3, number % 2)) from numbers(10);

select 'windowFunnel';
select hex(windowFunnelState(3600)(toDateTime('2023-01-01 00:00:00', 'UTC'), number % 2 = 0, number % 3 = 0)) from numbers(10);

select 'retention';
select hex(retentionState(number % 2 = 0, number % 3 = 0, number % 5 = 0)) from numbers(10);

select 'groupBitmap*';
select hex(groupBitmapState(number::UInt32)) from numbers(10);
select hex(groupBitmapAndState(bitmapBuild(cast([15] AS Array(UInt32)))));
select hex(groupBitmapOrState(bitmapBuild(cast([15] AS Array(UInt32)))));
select hex(groupBitmapXorState(bitmapBuild(cast([15] AS Array(UInt32)))));

select 'any/anyLast/anyHeavy/min/max';
select hex(anyState(number)) from numbers(10);
select hex(anyLastState(number)) from numbers(10);
select hex(anyHeavyState(number)) from numbers(100);
select hex(minState(number)) from numbers(10);
select hex(maxState(number)) from numbers(10);

select 'avg';
select hex(avgState(number::Float64)) from numbers(10);

select 'groupBitOr/groupBitAnd/groupBitXor';
select hex(groupBitOrState(number::UInt64)) from numbers(4) where ((number + 1) % 2) = 0;
select hex(groupBitAndState(number::UInt64)) from numbers(4) where ((number + 1) % 2) = 0;
select hex(groupBitXorState(number::UInt64)) from numbers(4) where ((number + 1) % 2) = 0;

select 'sparkbar';
select hex(sparkbarState(10)(number, number * 2)) from numbers(10);

select 'sequenceNextNode';
select hex(sequenceNextNodeState('forward', 'first_match')(toDateTime('2023-01-01 00:00:00', 'UTC') + number, number::String, number = 1, number = 1)) from numbers(10);

select 'intervalLengthSum';
select hex(intervalLengthSumState(number, number + 5)) from numbers(10);

select 'maxIntersections';
select hex(maxIntersectionsState(number, number + 5)) from numbers(10);
select hex(maxIntersectionsPositionState(number, number + 5)) from numbers(10);

select 'histogram';
select hex(histogramState(10)(number::Float64)) from numbers(100);

select 'groupArrayInsertAt';
select hex(groupArrayInsertAtState(number, number)) from numbers(10);

select 'groupArrayMoving';
select hex(groupArrayMovingAvgState(3)(number::Float64)) from numbers(10);
select hex(groupArrayMovingSumState(3)(number::Float64)) from numbers(10);

select 'sequenceMatch/sequenceMatchEvents/sequenceCount';
select hex(sequenceMatchState('(?1)(?2)')(toDateTime('2023-01-01 00:00:00', 'UTC') + number, number % 2 = 0, number % 3 = 0)) from numbers(10);
select hex(sequenceCountState('(?1)(?2)')(toDateTime('2023-01-01 00:00:00', 'UTC') + number, number % 2 = 0, number % 3 = 0)) from numbers(10);

select 'varSampStable/stddevSampStable/varPopStable/stddevPopStable';
select hex(varSampStableState(number::Float64)) from numbers(10);
select hex(stddevSampStableState(number::Float64)) from numbers(10);
select hex(varPopStableState(number::Float64)) from numbers(10);
select hex(stddevPopStableState(number::Float64)) from numbers(10);

select 'covarSampStable/covarPopStable';
select hex(covarSampStableState(number::Float64, number * 2::Float64)) from numbers(10);
select hex(covarPopStableState(number::Float64, number * 2::Float64)) from numbers(10);

select 'corrStable';
select hex(corrStableState(number::Float64, number * 2::Float64)) from numbers(10);

select 'quantile*/median*';
select hex(quantileState(0.5)(number::Float64)) from numbers(10);
select hex(quantileTimingState(0.5)(number::UInt64)) from numbers(10);
select hex(medianState(number::Float64)) from numbers(10);
select hex(medianTimingState(number::UInt64)) from numbers(10);

select 'quantileExact/medianExact';
select hex(quantileExactState(0.5)(number::Float64)) from numbers(10);
select hex(medianExactState(number::Float64)) from numbers(10);

select 'quantileTDigest/medianTDigest';
select hex(quantileTDigestState(0.5)(number::Float64)) from numbers(10);
select hex(medianTDigestState(number::Float64)) from numbers(10);

select 'simpleLinearRegression';
select hex(simpleLinearRegressionState(number::Float64, number * 2::Float64)) from numbers(10);

select 'exponentialMovingAverage';
select hex(exponentialMovingAverageState(0.5)(number, number::UInt64)) from numbers(10);

select 'uniqUpTo';
select hex(uniqUpToState(10)(number)) from numbers(5);

select 'boundingRatio';
select hex(boundingRatioState(number::Float64, number * 2::Float64)) from numbers(10) where number > 1;

select 'analysisOfVariance';
select hex(analysisOfVarianceState(number::Float64, number % 3)) from numbers(30);

select 'rankCorr';
select hex(rankCorrState(number::Float64, number * 2::Float64)) from numbers(10);

select 'stochasticLinearRegression/stochasticLogisticRegression';
select hex(stochasticLinearRegressionState(0.1, 0.0, 5, 'SGD')(number::Float64, number::Float64, number * 2::Float64)) from numbers(10);
select hex(stochasticLogisticRegressionState(0.1, 0.0, 5, 'SGD')(number % 2, number::Float64, number * 2::Float64)) from numbers(10);

select 'mannWhitneyUTest';
select hex(mannWhitneyUTestState(number::Float64, number % 2)) from numbers(20);

select 'studentTTest/welchTTest';
select hex(studentTTestState(number::Float64, number % 2)) from numbers(20);
select hex(welchTTestState(number::Float64, number % 2)) from numbers(20);

select 'singleValueOrNull';
select hex(singleValueOrNullState(number)) from numbers(2) where number > 0;

select 'meanZTest';
select hex(meanZTestState(0.7, 0.45, 0.95)(number::Float64, number::UInt64)) from numbers(10);

select 'varPop/varSamp/stddevPop/stddevSamp/covarPop/covarSamp/corr';
select hex(varPopState(number::Float64)) from numbers(10);
select hex(varSampState(number::Float64)) from numbers(10);
select hex(stddevPopState(number::Float64)) from numbers(10);
select hex(stddevSampState(number::Float64)) from numbers(10);
select hex(covarPopState(number::Float64, number * 2::Float64)) from numbers(10);
select hex(covarSampState(number::Float64, number * 2::Float64)) from numbers(10);
select hex(corrState(number::Float64, number * 2::Float64)) from numbers(10);

select 'largestTriangleThreeBuckets';
select hex(largestTriangleThreeBucketsState(3)(number::UInt64, number::Float64)) from numbers(10);

select 'kolmogorovSmirnovTest';
select hex(kolmogorovSmirnovTestState(number::Float64, number % 2)) from numbers(20);
