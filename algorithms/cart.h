#ifndef CART_H
#define CART_H

class CARTNode;
class IAlgorithmDataSource;

/** The CART class represents the CART algorithm, which serves to build decision trees from data to classify or
 * to perform regressions.  CART stands for Classification and Regression Trees.
 */
class CART
{
public:
    CART( const IAlgorithmDataSource& data );
protected:
    /** The root of the CART. */
    CARTNode m_root;
    const IAlgorithmDataSource& m_data;
};

#endif // CART_H
