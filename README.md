# SPLC2020-FeatureRevisionLocation

Artifacts for the paper "Locating Feature Revisions in Software Systems Evolving in Space and Time" submitted as full paper to the research track of SPLC 2020.

Each target system folder contains:
* the ground truth variants used as input for our feature revision location technique `Input_variants`.
* the ground truth variants with random configurations `Random_variants`.
* the results `Retrieved_results` containing csv file for each ground truth variant comparison (`compare_input_variants` and `compare_random_variants`) and the composed variants by the traces computed (`input_config_composed_variants` and `random_config_composed_variants`).
* an illustration of the introduced and changed features over the commits analyzed `Evolution_in_space_and_time`.
* a file `configurations.csv` containing the configuration of input variants.
* a file `features_report_each_project_commit.csv` containing the number of features introduces and changed for each commit analyzed.
* a file `randomconfigurations.csv` containing the configuration of random variants.
* a file `runtime.csv` containing the recorded runtime to extract traces for each input variant in milliseconds.

The implementation of the results shown in this paper is available in two repositories:
* [ECCO repository](https://github.com/jku-isse/ecco/tree/develop) contains the feature revision location technique.
* [git-ecco repository](https://github.com/GabrielaMichelon/git-ecco/tree/develop) contains the implementation for mining ground truth variants.

To reproduce the results of this paper you can clone this repository (which contains the artifacts used and the results presented in the paper) to your local directory and the [git-ecco repository](https://github.com/GabrielaMichelon/git-ecco/tree/develop), which contains the [ECCO repository](https://github.com/jku-isse/ecco/tree/develop) as a lib. Then you are able to run the process to create the ground truth variants and the process of feature revision location.

The steps to reproduce the results of the feature revision location are explained below:
* It is necessary to use the input variants (which are in the folder `Input_variants`) and the `configurations.csv` and `runtime.csv` files inside each target system folder (`LibSSH`, `Marlin`, `SQLite`). In case of reproducing the results of the random configurations you must use as input variants the variants from the folder `Random_variants` and the configuration of the file `randomconfigurations.csv`. The input variants are the ground truth variants used in this paper. However, if you desire to create variants as a ground truth instead of using the ones available in this repository, you can follow the steps in the [git-ecco repository](https://github.com/GabrielaMichelon/git-ecco/tree/develop).
* In the [git-ecco repository](https://github.com/GabrielaMichelon/git-ecco/tree/develop) repository there is a module [translation](https://github.com/GabrielaMichelon/git-ecco/tree/develop/translation) with a class [FeatureRevisionLocationTest.java](https://github.com/GabrielaMichelon/git-ecco/blob/develop/translation/src/test/java/at/jku/isse/gitecco/translation/test/FeatureRevisionLocationTest.java), which contains four necessary tests to obtain the results of this paper (note: you probably will need to edit the VM options in each test configuration, suggested VM options: "-Xss1000m -Xmx10g"): 
  * `TestEccoCommit`: the test for the feature revision location. 
  * `TestEccoCheckout`: the test for composing the variants with the mapping between feature revisions and artifacts.
  * `TestCompareVariants` the test for comparison of the ground truth variants with the composed variants.
  * `GetCSVInformationTotalTest` the test for computing the metrics to compute the results.
* For each target system folder (`LibSSH`, `Marlin`, `SQLite`) extract the files of the folder `Input_variants` (if it is compacted) and uses its directory string in the variable `resultsCSVs_path`.
* Use the string of the directory containing the file `configurations.csv` in the variable `configuration_path`.
* In the same folder of the target system create a new folder (named as `variant_results`) and use its directory string in the variable `resultMetrics_path`. In this new folder (`variant_results`) also create a new folder named as `checkout`.
* Each directory containing the folder of the artifacts must be defined in the variables corresponding to them `marlinFolder`, `libsshFolder` and `sqliteFolder`.
* The directory where will be stored the results of the metrics computed in the paper (precision, recall, f1 score and runtime) must be defined in the variable `metricsResultFolder`.
* Now, run the first three tests in the ordered mentioned above for each target system (`TestEccoCommit`, `TestEccoCheckout`, `TestCompareVariants`).
* Lastly, you can run the test to compute metrics and the results (`GetCSVInformationTotalTest`).

In case that you only want to compute the metrics (run the test `GetCSVInformationTotalTest`) you can skip the first three tests mentioned above and just copy the csv files (inside the system folder `Retrieved_results/compare_input_variants`) in the root folder of each target system cloned from this repository.
