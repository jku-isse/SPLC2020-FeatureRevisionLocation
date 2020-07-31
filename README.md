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

The implementation to reproduce the results is available in two repositories:
* [ECCO repository](https://github.com/jku-isse/ecco/tree/develop) contains the feature location revision technique.
* [git-ecco repository](https://github.com/GabrielaMichelon/git-ecco/tree/develop) contains the implementation for mining ground truth variants.
