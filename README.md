﻿# gammaray
[![Documentation Status](https://readthedocs.org/projects/gammaray/badge/?version=latest)](https://gammaray.readthedocs.io/en/latest/?badge=latest)
[![Build Status](https://travis-ci.com/PauloCarvalhoRJ/gammaray.svg?branch=master)](https://travis-ci.com/PauloCarvalhoRJ/gammaray)
                 
<center><img src="https://github.com/PauloCarvalhoRJ/gammaray/blob/master/frontpage.png"/></center>

Repository of the GammaRay geostatistics software.

Abstract:
GammaRay is a graphical user interface (GUI) that automates geostatistical workflows by driving and coordinating the several modules of the renowned Geostatistical Software Library (GSLib).  The main purpose of GammaRay is to add a user-friendly interface layer on top of the scientifically and numerically robust GSLib, greatly automating parameter file editing and module chaining so the practitioner can focus on geostatistics.  GammaRay was conceived as a free and portable alternative to WinGSLib®, which is a commercial software available only for Microsoft Windows® users.  GammaRay is also open source and thus is subject to code review and can receive contributions from other software developers as well as user feedback, suggestions and bug reports.  GammaRay is built with the C++ programming language upon the famous Qt library to leverage the construction of a modern and platform independent graphical user interface.<br><br>
GammaRay can be freely used in personal, academic and commercial applications, provided you give due credit to the authors.  You can cite it as:<br>
    CARVALHO, P. R. M.; MACHADO, P. L. et al. GammaRay: A graphical interface to GSLib and other geomodeling algorithms, (2014), GitHub repository, https://github.com/PauloCarvalhoRJ/gammaray<br><br>
If you enjoyed this project, you might also enjoy GeostatsPy: https://github.com/GeostatsGuy/GeostatsPy or https://geostatsguy.github.io/MachineLearningDemos_Book/intro.html (free e-book) and PyLPM: https://pylpm.readthedocs.io/en/latest/<br>

Python script to convert Eclipse grids to Paraview-compatible VTU format: https://github.com/BinWang0213/PyGRDECL

VERSION HISTORY:<br>
&nbsp;&nbsp;&nbsp;Version 6.22  - Drift analysis and drift model fitting to data.<br>
&nbsp;&nbsp;&nbsp;Version 6.20  - Contact Analysis.<br>
&nbsp;&nbsp;&nbsp;Version 6.18  - MCRFSim execution in batch/unattended mode; dependencies upgrades (VTK, ITK, Boost, C++14, ...).<br>
&nbsp;&nbsp;&nbsp;Version 6.17  - Transiography and MCRFSim for Bayesian approach; some fixes and improvements.<br>
&nbsp;&nbsp;&nbsp;Version 6.16  - Upgrade of VTK to 9.1 and other years-old dependencies; some fixes.<br>
&nbsp;&nbsp;&nbsp;Version 6.14  - Several improvements mainly involving the 3D Viewer and dataset processing.<br>
&nbsp;&nbsp;&nbsp;Version 6.12  - Several new methods to work with grids. Several fixes and enhancements.<br>
&nbsp;&nbsp;&nbsp;Version 6.9   - Export geologic grids as Eclipse grids, multiple other new features, enhancements and fixes.<br>
&nbsp;&nbsp;&nbsp;Version 6.7   - New data type: Geologic section.<br>
&nbsp;&nbsp;&nbsp;Version 6.6   - Mean, median and Gaussian filters, improvements and bug fixes.<br>
&nbsp;&nbsp;&nbsp;Version 6.5   - vertical proportion curves, improvements Automatic Variogram Fitting, enhancements and several fixes.<br>
&nbsp;&nbsp;&nbsp;Version 6.3   - improvements to data imputation with MCMC and parameter experiments for Automatic Variogram Fitting.<br>
&nbsp;&nbsp;&nbsp;Version 6.2   - data imputation with Markov Chains-Monte Carlo simulation and filtering of data sets.<br>
&nbsp;&nbsp;&nbsp;Version 6.1   - create facies transition matrices and 3D picking & probing.<br>
<strong>Version 6.0   - Markov Chain Random Field Simulation and numerous new features and enhancements<br></strong>
&nbsp;&nbsp;&nbsp;Version 5.7   - full 2d automatic variogram fitting and analysis<br>
&nbsp;&nbsp;&nbsp;Version 5.5   - wavelet transform<br>
&nbsp;&nbsp;&nbsp;Version 5.3   - Gabor analysis<br>
&nbsp;&nbsp;&nbsp;Version 5.1   - empirical mode decomposition<br>
<strong>Version 5.0   - stratigraphic grid<br></strong>
&nbsp;&nbsp;&nbsp;Version 4.9   - sequential indicator simulation<br>
&nbsp;&nbsp;&nbsp;Version 4.7   - factorial kriging<br>
&nbsp;&nbsp;&nbsp;Version 4.5.1 - several fixes<br>
&nbsp;&nbsp;&nbsp;Version 4.5   - variographic decomposition (experimental)<br>
&nbsp;&nbsp;&nbsp;Version 4.3.3 - several improvements<br>
&nbsp;&nbsp;&nbsp;Version 4.3   - quick varmap with FFT<br>
<strong>Version 4.0   - calculator scripting<br></strong>
&nbsp;&nbsp;&nbsp;Version 3.8   - singular value decomposition<br>
&nbsp;&nbsp;&nbsp;Version 3.6.1 - feature to delete variables from GEO-EAS files<br>
&nbsp;&nbsp;&nbsp;Version 3.6   - collocated cokriging<br>
&nbsp;&nbsp;&nbsp;Version 3.5   - classification and regression with CART and Random Forest<br>
&nbsp;&nbsp;&nbsp;Version 3.2   - several improvements<br>
<strong>Version 3.0   - sequential gaussian simulation<br></strong>
&nbsp;&nbsp;&nbsp;Version 2.7.2 - several fixes<br>
&nbsp;&nbsp;&nbsp;Version 2.7.1 - improvement and fix<br>
&nbsp;&nbsp;&nbsp;Version 2.7   - Image Jockey (FFT image manipulation)<br>
&nbsp;&nbsp;&nbsp;Version 2.5.1 - switch to 64-bit<br>
&nbsp;&nbsp;&nbsp;Version 2.5   - unvalued cells estimation and histogram over a realization ensemble<br>
&nbsp;&nbsp;&nbsp;Version 2.4   - variography over a realization ensemble<br>
&nbsp;&nbsp;&nbsp;Version 2.3   - grid resampling<br>
&nbsp;&nbsp;&nbsp;Version 2.2   - FFT<br>
&nbsp;&nbsp;&nbsp;Version 2.1   - optimizations<br>
<strong>Version 2.0   - 3D viewer<br></strong>
&nbsp;&nbsp;&nbsp;Version 1.7.1 - assorted improvements and fixes<br>
&nbsp;&nbsp;&nbsp;Version 1.7   - cokriging<br>
&nbsp;&nbsp;&nbsp;Version 1.6   - soft indicator calibration<br>
&nbsp;&nbsp;&nbsp;Version 1.5   - indicator kriging post-processing<br>
&nbsp;&nbsp;&nbsp;Version 1.4   - assorted improvements and fixes<br>
&nbsp;&nbsp;&nbsp;Version 1.3.1 - minor fixes<br>
&nbsp;&nbsp;&nbsp;Version 1.3   - indicator kriging<br>
&nbsp;&nbsp;&nbsp;Version 1.2.1 - removal of duplicate samples<br>
&nbsp;&nbsp;&nbsp;Version 1.2   - usability improvements<br>
&nbsp;&nbsp;&nbsp;Version 1.1   - icons for 4k displays<br>
&nbsp;&nbsp;&nbsp;Version 1.0.1 - minor fixes<br>
<strong>Version 1.0   - first usable version<br></strong>
