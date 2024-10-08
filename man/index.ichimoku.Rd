% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/methods.R
\name{index}
\alias{index}
\alias{index.ichimoku}
\title{Extract the Index of Ichimoku Objects}
\usage{
\method{index}{ichimoku}(x, subset, ...)
}
\arguments{
\item{x}{an object of class \sQuote{ichimoku}.}

\item{subset}{an integer or logical value or vector by which to subset the
index.}

\item{...}{arguments passed to or from other methods.}
}
\value{
The date-time index of the ichimoku object as a vector of POSIXct
    values.
}
\description{
Method for extracting the date-time index of ichimoku objects.
}
\details{
This function is an S3 method for the generic function index() for
    class \sQuote{ichimoku}. It can be invoked by calling index(x) on an
    object \sQuote{x} of class \sQuote{ichimoku}.

    Subsetting by specifying the \sQuote{subset} parameter subsets using the
    numerical values underlying the POSIXct times and results in a faster
    operation than usual subset operators such as \sQuote{\[}.
}
\examples{
cloud <- ichimoku(sample_ohlc_data)
index(cloud)[101:110]
index(cloud, 101:110)

}
