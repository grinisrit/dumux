# Determining the Darcy permeability of a pore network

__In this example, you will learn how to__

* simulate flow on a pore network by applying a pressure gradient in a given direction
* perform an upscaling in order to determine the intrinsic single-phase permeability $`\mathbf{K}`$ [m$`^2`$]


__Result__.
As a result of the simulation of this example, you will get the intrinsic single-phase permeabilities for each spatial direction $`K_{xx}`$, $`K_{yy}`$, $`K_{zz}`$ [m$`^2`$]
as direct output on your terminal.
Additionally, the output contains the values of the auxiliary parameters: cross-sectional area, mass flux integrated over the cross-sectional area,
and the resulting Darcy velocity as in the example below for the x-direction permeability $`K_{xx}`$:

```

x-direction: Area = 1.600000e-07 m^2; Massflux = 4.509338e-13 kg/s; v_Darcy = 2.818337e-09 m/s; K = 2.818337e-13 m^2

```

The pressure distribution throughout the pore network as well as pore-network characteristics will also be written to a vtp output file that can be viewed with ParaView.
Figure 1 shows the pressure distribution within the pore network for the case of flow in x-direction.

<figure>
    <center>
        <img src="img/result.png" alt="Pore-network pressure distribution" width="60%"/>
        <figcaption> <b> Fig.1 </b> - Pressure distribution within the pore network for flow in x-direction. </figcaption>
    </center>
</figure>

__Table of contents__. This description is structured as follows:

[[_TOC_]]


## Problem setup

We consider a single-phase problem within a randomly generated pore network of 20x20x20 pores cubed from which some of the pore throats were [deleted randomly](https://doi.org/10.1029/2010WR010180).
The inscribed pore body radii follow a truncated log-normal distribution.
To calculate the upscaled permeability, a pressure difference of 4000 Pa is applied sequentially in every direction, while all lateral sides are closed.
The resulting mass flow rate is then used  to determine $`\mathbf{K}`$, as described [later](upscalinghelper.md).

## Mathematical and numerical model

In this example we are using the single-phase pore-network model of DuMu<sup>x</sup>, which considers a Hagen-Poiseuille-type law to relate the volume flow from on pore body to another to discrete pressure drops $`\Delta p = p_i - p_j`$ between the pore bodies.
We require mass conservation at each pore body $`i`$:

```math
 \sum_j Q_{ij} = 0,
```
where $`Q_{ij}`$ is the discrete volume flow rate in a throat connecting pore bodies $`i`$ and $`j`$:

```math
 Q_{ij} = g_{ij} (p_i - p_j).
```

# Implementation & Post processing

In the following, we take a closer look at the source files for this example.
We will discuss the different parts of the code in detail subsequently.

```
????????? pnmpermeabilityupscaling/
    ????????? CMakeLists.txt          -> build system file
    ????????? main.cc                 -> main program flow
    ????????? params.input            -> runtime parameters
    ????????? properties.hh           -> compile time settings for the simulation
    ????????? problem.hh              -> boundary & initial conditions for the simulation
    ????????? spatialparams.hh        -> parameter distributions for the simulation
    ????????? upscalinghelper.hh      -> helper for the upscaling calculations and writing the results
```
