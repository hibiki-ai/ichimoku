% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/strat.R
\name{stratcombine}
\alias{stratcombine}
\title{Combine Ichimoku Strategies}
\usage{
stratcombine(s1, s2)
}
\arguments{
\item{s1}{an ichimoku object containing a strategy.}

\item{s2}{an ichimoku object containing a strategy.}
}
\value{
An ichimoku object augmented with the combined strategy.
}
\description{
Create custom combined strategies from existing strategies contained in 's1'
    and 's2' to form 's1 & s2'.
}
\details{
The combined strategy 's1 & s2' means indicator conditions in 's1'
    and 's2' have to be met simulateneously for a trade position to be taken.

    The boolean values showing whether these conditions are met are stored in
    the 'cond' column.

    The stategy summary may be accessed by the summary() function or via
    look().
}
\section{Further Details}{

    Please refer to the strategies vignette by running:
    \code{vignette("strategies", package = "ichimoku")}
}

\examples{
cloud <- ichimoku(sample_ohlc_data, ticker = "TKR")
strat1 <- strat(cloud, c1 = "close", c2 = "kijun")
strat2 <- strat(cloud, c1 = "cloudBase", c2 = "tenkan")
cstrat <- stratcombine(strat1, strat2)
summary(cstrat)
plot(cstrat)

}