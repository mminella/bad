# Models

Our predicte models for the various strategies.

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
