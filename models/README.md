# Models

Our predicte models for the various strategies. We currently try to predict the
following client interactions:

* `Read All` -- read all records in sorted order / sort.
* `First` -- retrieve the first record in sorted order.
* `Nth` -- retrieve the nth record in sorted order.
* `CDF` -- retrieve each of the 1th, 2nd, 3rd, ... 45th, 46th, ... 99th
  percentile records.

The final operation tries to predict an interaction style that involves
multiple operations with the system potentially and so will ammortize any
startup costs.

## Running

You'll need R. After installing and getting R working, just run:

```
./install-packages.R
```

And that should install all the required R packages to run the models.

## machines.csv

We store the needed machine performance parameters in the csv file. Two are
currently provided. `machines.csv` which contains the values that we measured,
and `machines_advertised.csv`, which contains the values that Amazon promises
as a lower bound (for disk I/O only, no promises are made for other
components).
