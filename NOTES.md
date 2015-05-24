# Various notes

## Storage Systems

* LUN: storage device.

* Storage system models are the reverse of what we want: They take a fixed
  workload and try to predict (either in absolute or relative terms) how some
  performance metric will be affected by moving the workload from one storage
  device to another.
  - We instead want to predict (in either absolute or relative terms) how
    different workloads will perform on the same storage system.
  - Question: If I have one do I have the other? Are they equivalent or
    different?
  - It seems difficult to compare them. With storage modelling, I'm taking the
    same workload and moving it. Performance is relative straight-forward to
    talk about. However, when contrasting two different workloads, what does
    performance (especially if relative, not absolute) mean?

## Models

Aim is to allow measuring an app in isolation, and an IO device in isolation
and then be able to predict performance.

* App has X characteristics (demand)
* IO device has Y characteristics (supply)
* Running X on Y should behave as Z (performance)

* What is Z?
  - Performance metrics that the end-user cares about.
  - Total running-time (throughput)
  - Running-time of sub-operations (latency)
  - ... etc

* What do I need from X and Y to accurately predict Z?
* How do I combine (+) X & Y to get Z? Out model...

* Model:
  - measure1 :: IO -> Y
  - measure2 :: App -> X
  - model:: Y -> X -> Z

* App metrics (workload characteristics):
  - Really want to know what they are when the app is run on the storage system
    you are predicting performance over (Xi). However, since we are trying to
    avoid doing just this, we often use workload chracteristics obtained from
    running on a different storage system (Xj).
  - Many models then make the assumption that `Xi = Xj` and proceed from there.
  - Others use `Xj` to predict, reaching `Xi = F(Xj)`.

* BASIL:
  - measure1:
    - Either the 750 configurations to get numbers for the 4 workload metrics
      (absolute).
    - Or, the slope parameter (relative).
  - measure2:
    - Single values for the 4 workload metrics: IO Size, Outstanding IO, read%,
      random%.
  - model: linear fit.

## Black-box vs White-box

* Black-box: Doesn't use any internal details of the device being modelled.
* White-box: Makes use of knowledge of internals.

* Black-box:
  - Simplest example: numeric average, e.g., miles per gallon.
  - Can extend with 'workload characteristics,' e.g., highway or city.
  - Can think of workload characteristics as a way to index into the model.

## Disk Performance Metrics

* Read-write ratio
* IO Size
* IO Latency
* Spatial Locality
* IO Inter-arrival Time
* Active queue depth (outstanding IOs)
* Burstiness
* Spatio-temporal  correlation

