struct base 
{
    virtual void foo() = 0;
};

class zoo : virtual public base
{
    virtual void foo() override {}
};

class bar : virtual public base
{
    virtual void zar() = 0;
};

class some : public zoo, public bar {};
